#include "game_profile.h"
#include <string.h>

extern const GameProfile GAME_FH2;
extern const GameProfile GAME_FH3;

/* Add future stable profiles here only after their hashes/behavior are validated. */
static const GameProfile *const profiles[] = {
    &GAME_FH2,
    &GAME_FH3,
};

static const GameProfile *volatile active_profile;

static BOOL hash_in_list(const DWORD hash[4], const ShaderHash *list, UINT count) {
    for (UINT i = 0; i < count; ++i) {
        if (!memcmp(hash, list[i].v, 16)) {
            return TRUE;
        }
    }
    return FALSE;
}

static int profile_role_for_hash(const GameProfile *profile, const DWORD hash[4]) {
    if (hash_in_list(hash, profile->mv_ps_hashes, profile->mv_ps_hash_count)) {
        return ROLE_MV;
    }
    if (hash_in_list(hash, profile->resolve_ps_hashes, profile->resolve_ps_hash_count)) {
        return ROLE_RESOLVE;
    }
    return ROLE_NONE;
}

const GameProfile *game_profile_active(void) {
    return (const GameProfile *)InterlockedCompareExchangePointer(
        (PVOID volatile *)&active_profile, NULL, NULL);
}

const GameProfile *game_profile_match(const D3D12_SHADER_BYTECODE *ps, int *role) {
    if (role) {
        *role = ROLE_NONE;
    }
    if (!ps || !ps->pShaderBytecode || ps->BytecodeLength < 20) {
        return NULL;
    }

    const BYTE *bytes = (const BYTE *)ps->pShaderBytecode;
    if (memcmp(bytes, "DXBC", 4)) {
        return NULL;
    }

    DWORD hash[4];
    memcpy(hash, bytes + 4, 16);

    const GameProfile *active = game_profile_active();
    if (active) {
        int found_role = profile_role_for_hash(active, hash);
        if (found_role != ROLE_NONE) {
            if (role) {
                *role = found_role;
            }
            return active;
        }
        return NULL;
    }

    for (UINT i = 0; i < (UINT)(sizeof(profiles) / sizeof(profiles[0])); ++i) {
        const GameProfile *candidate = profiles[i];
        int found_role = profile_role_for_hash(candidate, hash);
        if (found_role == ROLE_NONE) {
            continue;
        }

        const GameProfile *selected = (const GameProfile *)InterlockedCompareExchangePointer(
            (PVOID volatile *)&active_profile, (PVOID)candidate, NULL);
        if (!selected) {
            selected = candidate;
        }
        if (selected != candidate) {
            return NULL;
        }
        if (role) {
            *role = found_role;
        }
        return candidate;
    }
    return NULL;
}

BOOL game_profile_is_jitter_depth(const GameProfile *profile, DXGI_FORMAT format) {
    if (!profile) {
        return FALSE;
    }
    for (UINT i = 0; i < profile->jitter_depth_format_count; ++i) {
        if (profile->jitter_depth_formats[i] == format) {
            return TRUE;
        }
    }
    return FALSE;
}
