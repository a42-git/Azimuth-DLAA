/* NVIDIA DLAA (DLSS Super Resolution at 1:1, no upscaling) through NVIDIA NGX.

   The NGX static library is built with MSVC and cannot be linked into this MinGW/Zig-built
   dxgi.dll, so all NGX calls live in a small separate DLL (azimuth_ngx.dll, see bridge/) that is
   loaded here at runtime. nvngx_dlss.dll (NVIDIA) must sit next to azimuth.exe as well.

   This file keeps the exact same job (and the same internal API) the FSR backend had:
   jitter sequence, feature creation, dispatch, copy of the result back into the game's
   scene colour. hooks.c is unchanged apart from names. */
#define COBJMACROS
#include "common.h"
#include <stdbool.h>
#include <string.h>

/* Exports of azimuth_ngx.dll. */
typedef int (*PfnNgxInit)(ID3D12Device *dev, const wchar_t *dir, int *ngx_result);
typedef int (*PfnNgxEvaluate)(ID3D12GraphicsCommandList *cl, ID3D12Resource *color, ID3D12Resource *depth,
                              ID3D12Resource *mv, ID3D12Resource *out, unsigned w, unsigned h, float jx, float jy,
                              float mv_scale_x, float mv_scale_y, int reset, int *ngx_result);
typedef const char *(*PfnNgxResultString)(int ngx_result);

static PfnNgxInit p_init;
static PfnNgxEvaluate p_evaluate;
static PfnNgxResultString p_result_string;
static int load_failed;
static int ngx_ready;
static int ngx_failed;

static ID3D12Device *device;
static UINT ctx_w, ctx_h;
static ID3D12Resource *out_tex;
static int out_failed;

static volatile LONG enabled = 1;
static int need_reset = 1;
static int jitter_live;              /* DLSS ran last frame, so jitter the scene */
static const int phase_count = 16;   /* DLAA: render == display, 16 Halton phases is plenty */
static int jitter_index;
static float jx, jy;                 /* jitter for the frame being recorded */
static int jitter_used;              /* the scene of this frame was rendered with (jx, jy) */

static void exe_dir(wchar_t *path) {
    DWORD n = GetModuleFileNameW(NULL, path, MAX_PATH);
    while (n && path[n - 1] != L'\\') {
        --n;
    }
    path[n] = 0;
}

static int file_exists(const wchar_t *p) {
    DWORD a = GetFileAttributesW(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static int load(void) {
    if (p_evaluate) {
        return 1;
    }
    if (load_failed) {
        return 0;
    }
    load_failed = 1;
    wchar_t dir[MAX_PATH], path[MAX_PATH + 32];
    exe_dir(dir);

    lstrcpyW(path, dir);
    lstrcatW(path, L"nvngx_dlss.dll");
    if (!file_exists(path)) {
        log_msg("DLSS: nvngx_dlss.dll not found next to azimuth.exe");
        return 0;
    }
    lstrcpyW(path, dir);
    lstrcatW(path, L"azimuth_ngx.dll");
    HMODULE m = LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!m) {
        log_msg("DLSS: azimuth_ngx.dll not found next to azimuth.exe (error %lu)", GetLastError());
        return 0;
    }
    *(FARPROC *)&p_init = GetProcAddress(m, "azngx_init");
    *(FARPROC *)&p_evaluate = GetProcAddress(m, "azngx_evaluate");
    *(FARPROC *)&p_result_string = GetProcAddress(m, "azngx_result_string");
    if (!p_init || !p_evaluate || !p_result_string) {
        log_msg("DLSS: azimuth_ngx.dll exports missing");
        p_evaluate = NULL;
        return 0;
    }
    load_failed = 0;
    return 1;
}

void dlss_init(ID3D12Device *d) {
    device = d;
}

int dlss_enabled(void) {
    return enabled;
}

void dlss_toggle(void) {
    enabled = !enabled;
    need_reset = 1;
    jitter_live = 0;
    log_msg("DLAA %s", enabled ? "ON" : "OFF");
}


int dlss_jitter(float *x, float *y) {
    if (!enabled || !jitter_live || !ngx_ready) {
        return 0;
    }
    *x = jx;
    *y = jy;
    jitter_used = 1;
    return 1;
}

static float halton(int i, int base) {
    float f = 1.0f, r = 0.0f;
    while (i > 0) {
        f /= (float)base;
        r += f * (float)(i % base);
        i /= base;
    }
    return r;
}

static void next_jitter(void) {
    jitter_index = (jitter_index + 1) % phase_count;
    jx = halton(jitter_index + 1, 2) - 0.5f;
    jy = halton(jitter_index + 1, 3) - 0.5f;
}

static int ensure_ready(UINT w, UINT h) {
    if (ngx_ready && ctx_w == w && ctx_h == h && out_tex) {
        return 1;
    }
    if (ngx_failed || out_failed || !device || !load()) {
        return 0;
    }
    if (!ngx_ready) {
        wchar_t dir[MAX_PATH];
        exe_dir(dir);
        int r = 0;
        if (!p_init(device, dir, &r)) {
            log_msg("DLSS: NGX init failed (%s, 0x%08x). Needs an NVIDIA RTX GPU, a recent driver and nvngx_dlss.dll.",
                    p_result_string(r), (unsigned)r);
            ngx_failed = 1;
            return 0;
        }
        log_msg("DLSS: NGX initialised, DLAA available");
        ngx_ready = 1;
    }
    if (out_tex) {
        /* Size change: in-flight work may still use the old texture, so it is leaked rather
           than released mid-frame. Rare (only when the render size changes). */
        log_msg("DLSS: render size changed to %ux%u", w, h);
        out_tex = NULL;
    }
    D3D12_HEAP_PROPERTIES hp;
    memset(&hp, 0, sizeof(hp));
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC rd;
    memset(&rd, 0, sizeof(rd));
    rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rd.Width = w;
    rd.Height = h;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.Format = DXGI_FORMAT_R11G11B10_FLOAT;
    rd.SampleDesc.Count = 1;
    rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    if (FAILED(ID3D12Device_CreateCommittedResource(device, &hp, D3D12_HEAP_FLAG_NONE, &rd,
                                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL,
                                                    &IID_ID3D12Resource, (void **)&out_tex))) {
        log_msg("DLSS: output texture creation failed");
        out_tex = NULL;
        out_failed = 1;
        return 0;
    }
    ID3D12Resource_SetName(out_tex, L"AzimuthDLAA Output");
    ctx_w = w;
    ctx_h = h;
    need_reset = 1;
    return 1;
}

#define READ_STATE (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)

int dlss_run(ID3D12GraphicsCommandList *cl, ID3D12Resource *color, D3D12_RESOURCE_STATES color_state,
             ID3D12Resource *depth, D3D12_RESOURCE_STATES depth_state, ID3D12Resource *motion,
             D3D12_RESOURCE_STATES motion_state) {
    if (!enabled) {
        return 0;
    }
    D3D12_RESOURCE_DESC cd;
    color->lpVtbl->GetDesc(color, &cd);
    UINT w = (UINT)cd.Width, h = cd.Height;
    if (!ensure_ready(w, h)) {
        return 0;
    }
    /* Everything DLSS reads goes to a shader-read state. */
    if (color_state != READ_STATE) {
        hooks_transition(cl, color, color_state, READ_STATE);
    }
    if (depth_state != READ_STATE) {
        hooks_transition(cl, depth, depth_state, READ_STATE);
    }
    if (motion_state != READ_STATE) {
        hooks_transition(cl, motion, motion_state, READ_STATE);
    }
    int r = 0;
    /* Our motion vectors are UV-space offsets from current to previous position, so the scale
       to pixels is the render size (same convention as the FSR version). */
    int ok = p_evaluate(cl, color, depth, motion, out_tex, w, h, jitter_used ? jx : 0.0f, jitter_used ? jy : 0.0f,
                        (float)w, (float)h, need_reset != 0, &r);
    if (ok) {
        hooks_transition(cl, color, READ_STATE, D3D12_RESOURCE_STATE_COPY_DEST);
        hooks_transition(cl, out_tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        cl->lpVtbl->CopyResource(cl, color, out_tex);
        hooks_transition(cl, out_tex, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        hooks_transition(cl, color, D3D12_RESOURCE_STATE_COPY_DEST, READ_STATE);
    } else {
        static int warned;
        if (warned++ < 5) {
            log_msg("DLSS: evaluate failed (%s, 0x%08x)", p_result_string(r), (unsigned)r);
        }
    }
    if (color_state != READ_STATE) {
        hooks_transition(cl, color, READ_STATE, color_state);
    }
    if (depth_state != READ_STATE) {
        hooks_transition(cl, depth, READ_STATE, depth_state);
    }
    if (motion_state != READ_STATE) {
        hooks_transition(cl, motion, READ_STATE, motion_state);
    }
    if (ok) {
        static int once;
        if (!once) {
            once = 1;
            log_msg("DLAA active at %ux%u", w, h);
        }
        need_reset = 0;
        /* Next frame's jitter. Advancing here ties the sequence to rendered scenes. */
        next_jitter();
        jitter_used = 0;
    }
    return ok;
}

void dlss_end_frame(int ran) {
    jitter_live = ran && enabled;
    if (!ran) {
        need_reset = 1;
        jitter_used = 0;
    }
}
