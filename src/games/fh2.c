#include "../game_profile.h"

static const char FH2_MV_PS_HLSL[] =
    "Texture2D<float> txWorldDepth : register(t0);\n"
    "Texture2D<uint> txWorldStencil : register(t1);\n"
    "cbuffer ImageProcPostProcessConstBuffer : register(b1) { float4 cb1[557]; };\n"
    "cbuffer AzimuthDynamicIndex : register(b13) { uint4 cb13; };\n"
    "float4 row(uint i, uint sel) { return i < 542u ? cb1[i + sel] : float4(0, 0, 0, 0); }\n"
    "float4 main(float4 pos : SV_POSITION, float2 uv : TexCoord0) : SV_TARGET {\n"
    "    uint w, h;\n"
    "    txWorldDepth.GetDimensions(w, h);\n"
    "    int2 p = min(int2(uv.xy * float2(w, h)), int2(w - 1, h - 1));\n"
    "    float d = txWorldDepth.Load(int3(p, 0));\n"
    "    uint s = txWorldStencil.Load(int3(p, 0)) << 2;\n"
    "    uint sel = (cb13.x >> 4) & 15u;\n"
    "    float2 ndc = uv.xy * float2(2, -2) + float2(-1, 1);\n"
    "    float3 r = ndc.x * row(s + 30u, sel).xyw + ndc.y * row(s + 31u, sel).xyw\n"
    "             + d * row(s + 32u, sel).xyw + row(s + 33u, sel).xyw;\n"
    "    float2 prev = r.z != 0 ? r.xy / r.z : ndc;\n"
    "    float2 m = prev - ndc;\n"
    "    return float4(m.x * 0.5, -m.y * 0.5, 0, 0);\n"
    "}\n";

static const ShaderHash FH2_MV_HASHES[] = {
    {.v = {0x8a12870f, 0x60f7c6e9, 0x21d28459, 0x07b63a84}},
};

static const ShaderHash FH2_RESOLVE_HASHES[] = {
    {.v = {0x1549a8f9, 0xd8526d4b, 0x60608dd5, 0x6ae7bb32}},
};

const GameProfile GAME_FH2 = {
    .id = "fh2",
    .name = "Forza Horizon 2",
    .tested_azimuth = "1.0.3 / 1.0.4",
    .mv_ps_hashes = FH2_MV_HASHES,
    .mv_ps_hash_count = (UINT)(sizeof(FH2_MV_HASHES) / sizeof(FH2_MV_HASHES[0])),
    .resolve_ps_hashes = FH2_RESOLVE_HASHES,
    .resolve_ps_hash_count = (UINT)(sizeof(FH2_RESOLVE_HASHES) / sizeof(FH2_RESOLVE_HASHES[0])),
    .mv_ps_hlsl = FH2_MV_PS_HLSL,
    .mv_resolution_scale = 2.0f,
    .resolve_rtv_format = DXGI_FORMAT_R11G11B10_FLOAT,
    .resolve_dsv_format = DXGI_FORMAT_D32_FLOAT,
    .jitter_depth_formats = {
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
        DXGI_FORMAT_R32G8X24_TYPELESS,
    },
    .jitter_depth_format_count = 2,
    .dispatch_indexed_resolve = FALSE,
    .capture_sets_write_states = FALSE,
};
