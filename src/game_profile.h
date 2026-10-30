#pragma once

#include <windows.h>
#include <d3d12.h>
#include <dxgiformat.h>

enum {
    ROLE_NONE = 0,
    ROLE_MV = 1,
    ROLE_RESOLVE = 2,
};

typedef struct ShaderHash {
    DWORD v[4];
} ShaderHash;

typedef struct GameProfile {
    const char *id;
    const char *name;
    const char *tested_azimuth;

    const ShaderHash *mv_ps_hashes;
    UINT mv_ps_hash_count;
    const ShaderHash *resolve_ps_hashes;
    UINT resolve_ps_hash_count;
    const char *mv_ps_hlsl;

    float mv_resolution_scale;
    DXGI_FORMAT resolve_rtv_format;
    DXGI_FORMAT resolve_dsv_format;

    DXGI_FORMAT jitter_depth_formats[4];
    UINT jitter_depth_format_count;

    BOOL dispatch_indexed_resolve;
    BOOL capture_sets_write_states;
} GameProfile;

/* Matches a DXBC pixel shader against all known game profiles. The first match
   locks the process to that profile so hashes from another game can never become
   active later in the same process. */
const GameProfile *game_profile_match(const D3D12_SHADER_BYTECODE *ps, int *role);
const GameProfile *game_profile_active(void);
BOOL game_profile_is_jitter_depth(const GameProfile *profile, DXGI_FORMAT format);
