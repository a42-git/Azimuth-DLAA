// fh2_ngx.dll - the NVIDIA NGX (DLSS) half of FH2-DLAA.
//
// NVIDIA's nvsdk_ngx_*.lib is an MSVC static library, so it can't be linked into the
// Zig/MinGW-built dxgi.dll. This tiny DLL is built with MSVC instead and exposes a plain C
// interface that dxgi.dll loads at runtime.
//
// Build: see build_bridge.bat
#include <windows.h>
#include <d3d12.h>
#include <cstdio>
#include <cstring>

#include "nvsdk_ngx.h"
#include "nvsdk_ngx_helpers.h"

#define EXPORT extern "C" __declspec(dllexport)

static ID3D12Device *g_device;
static NVSDK_NGX_Parameter *g_params;     // owned by NGX, not released by us
static NVSDK_NGX_Handle *g_feature;
static unsigned g_w, g_h;
static bool g_ready;
static unsigned long long g_calls;

// Features replaced by a resize are released a few frames later, never mid-flight.
struct Retired {
    NVSDK_NGX_Handle *h;
    unsigned long long at;
};
static Retired g_retired[4];

static void release_retired(bool force) {
    for (auto &r : g_retired) {
        if (r.h && (force || g_calls - r.at > 16)) {
            NVSDK_NGX_D3D12_ReleaseFeature(r.h);
            r.h = nullptr;
        }
    }
}

EXPORT const char *fh2ngx_result_string(int result) {
    static char buf[160];
    const wchar_t *w = GetNGXResultAsString((NVSDK_NGX_Result)result);
    if (!w || !WideCharToMultiByte(CP_UTF8, 0, w, -1, buf, sizeof(buf), nullptr, nullptr)) {
        return "unknown";
    }
    return buf;
}

// Returns 1 when DLSS Super Resolution is usable. `dir` is the folder holding nvngx_dlss.dll
// (the Azimuth folder); NGX also uses it as its data/log path.
EXPORT int fh2ngx_init(ID3D12Device *device, const wchar_t *dir, int *ngx_result) {
    if (g_ready) {
        return 1;
    }
    static const wchar_t *paths[1];
    paths[0] = dir;
    NVSDK_NGX_FeatureCommonInfo info = {};
    info.PathListInfo.Path = paths;
    info.PathListInfo.Length = 1;

    NVSDK_NGX_Result r = NVSDK_NGX_D3D12_Init_with_ProjectID("b4c6e1f2-3a7d-4e59-8c21-6f0a9d2e7b13",
                                                             NVSDK_NGX_ENGINE_TYPE_CUSTOM, "1.0", dir, device, &info);
    *ngx_result = (int)r;
    if (NVSDK_NGX_FAILED(r)) {
        return 0;
    }
    r = NVSDK_NGX_D3D12_GetCapabilityParameters(&g_params);
    *ngx_result = (int)r;
    if (NVSDK_NGX_FAILED(r) || !g_params) {
        NVSDK_NGX_D3D12_Shutdown1(device);
        return 0;
    }
    int needs_driver = 0, available = 0;
    g_params->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needs_driver);
    g_params->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &available);
    if (needs_driver || !available) {
        *ngx_result = (int)NVSDK_NGX_Result_FAIL_OutOfDate;
        NVSDK_NGX_D3D12_Shutdown1(device);
        return 0;
    }
    g_device = device;
    g_ready = true;
    return 1;
}

// Runs DLAA: colour -> out, all at the same size. Feature creation is recorded into the same
// command list on first use and whenever the size changes.
EXPORT int fh2ngx_evaluate(ID3D12GraphicsCommandList *cl, ID3D12Resource *color, ID3D12Resource *depth,
                           ID3D12Resource *mv, ID3D12Resource *out, unsigned w, unsigned h, float jx, float jy,
                           float mv_scale_x, float mv_scale_y, int reset, int *ngx_result) {
    if (!g_ready) {
        *ngx_result = (int)NVSDK_NGX_Result_FAIL_NotInitialized;
        return 0;
    }
    ++g_calls;
    release_retired(false);

    if (!g_feature || w != g_w || h != g_h) {
        if (g_feature) {
            for (auto &r : g_retired) {
                if (!r.h) {
                    r.h = g_feature;
                    r.at = g_calls;
                    break;
                }
            }
            g_feature = nullptr;  // (if the ring was full the old one is leaked; resizes are rare)
        }
        NVSDK_NGX_DLSS_Create_Params cp = {};
        cp.Feature.InWidth = w;
        cp.Feature.InHeight = h;
        cp.Feature.InTargetWidth = w;
        cp.Feature.InTargetHeight = h;
        cp.Feature.InPerfQualityValue = NVSDK_NGX_PerfQuality_Value_DLAA;
        cp.InFeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_IsHDR | NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
        NVSDK_NGX_Result r = NGX_D3D12_CREATE_DLSS_EXT(cl, 1, 1, &g_feature, g_params, &cp);
        if (NVSDK_NGX_FAILED(r)) {
            *ngx_result = (int)r;
            g_feature = nullptr;
            return 0;
        }
        g_w = w;
        g_h = h;
        reset = 1;
    }

    NVSDK_NGX_D3D12_DLSS_Eval_Params ep = {};
    ep.Feature.pInColor = color;
    ep.Feature.pInOutput = out;
    ep.Feature.InSharpness = 0.0f;
    ep.pInDepth = depth;
    ep.pInMotionVectors = mv;
    ep.InJitterOffsetX = jx;
    ep.InJitterOffsetY = jy;
    ep.InReset = reset;
    ep.InMVScaleX = mv_scale_x;
    ep.InMVScaleY = mv_scale_y;
    ep.InRenderSubrectDimensions.Width = w;
    ep.InRenderSubrectDimensions.Height = h;
    ep.InPreExposure = 1.0f;
    ep.InExposureScale = 1.0f;
    NVSDK_NGX_Result r = NGX_D3D12_EVALUATE_DLSS_EXT(cl, g_feature, g_params, &ep);
    *ngx_result = (int)r;
    return NVSDK_NGX_SUCCEED(r) ? 1 : 0;
}

EXPORT void fh2ngx_shutdown(void) {
    if (!g_ready) {
        return;
    }
    release_retired(true);
    if (g_feature) {
        NVSDK_NGX_D3D12_ReleaseFeature(g_feature);
        g_feature = nullptr;
    }
    NVSDK_NGX_D3D12_Shutdown1(g_device);
    g_ready = false;
}
