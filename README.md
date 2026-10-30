# Azimuth-DLAA

NVIDIA DLAA support for Forza games running through [Azimuth](https://almost-there-games.itch.io/azimuth).

Azimuth-DLAA uses one shared D3D12/NGX core and small per-game render profiles. The current build automatically detects the supported game from its known render-pass shaders; users do not need separate FH2 and FH3 DLLs.

## Supported games

| Game | Status | Validated Azimuth |
| --- | --- | --- |
| Forza Horizon 2 | Supported | 1.0.3, 1.0.4 |
| Storm Island | Supported | 1.0.3, 1.0.4 |
| Forza Horizon 2 Presents Fast & Furious | Supported | 1.0.3, 1.0.4 |
| Forza Horizon 3 | Supported | 1.0.4 |
| Forza Motorsport 5 | Planned / not supported yet | - |
| Forza Motorsport 6 | Planned / not supported yet | - |

## Installation

1. Install Azimuth and make sure the game launches normally first.
2. Copy `dxgi.dll` and `azimuth_ngx.dll` next to `azimuth.exe`.
3. Provide `nvngx_dlss.dll` from an official NVIDIA DLSS SDK/runtime source if it is not included in the package you are using.
4. Set anti-aliasing to **Off** in Azimuth.
5. Launch a supported game.

The mod automatically selects the correct game profile. `azimuth_dlaa.log` records which profile was detected.

## Controls

- **F11** - toggle DLAA on/off.

F11 works during gameplay and photo mode. If a photo-mode frame is completely stationary, the visible result may not update until the game redraws; moving the photo camera slightly forces a redraw.

## Optional NVIDIA Profile Inspector tweaks

These settings are not required for Azimuth-DLAA to work.

DLAA presets can be forced by adding azimuth.exe to NVIDIA Profile Inspector and changing the DLSS preset there.

At 4K, I found preset E (CNN) and K (Transformer Gen 1) are good choice while preset M (Transformer Gen 2) is too heavy with close to no difference.

Results may vary depending on resolution.

I also saw a substantial performance improvement on my system (RTX 4070 Ti Super + Ryzen 5600) by forcing Resizable BAR / ReBAR for the Azimuth.exe profile.

## Architecture

The source is deliberately split so adding another game does not require another fork of the mod:

```text
src/
  hooks.c              shared D3D12 interception, MV replay and resolve dispatch
  dlss.c               shared DLAA/temporal state and NGX bridge client
  game_profile.h       per-game contract
  game_profiles.c      registry + runtime profile selection
  games/
    fh2.c              FH2 hashes, MV reconstruction and compatibility flags
    fh3.c              FH3 hashes, MV reconstruction and compatibility flags
bridge/
  ngx_bridge.cpp       shared NVIDIA NGX/DLSS bridge
```

A game profile owns only the things that are genuinely game-specific: exact motion-vector/resolve shader hashes, the motion-vector reconstruction shader, expected resolve formats, jitter depth formats, resolution scale, and any small compatibility flags.

The shared core does not identify games by executable filename. It locks onto the first exact known game shader it sees and will not switch profiles later in the process. This keeps unrelated profiles isolated.

For FM5/FM6, the intended workflow is to discover their render passes in a separate diagnostic branch and then add `src/games/fm5.c` / `fm6.c` plus one registry entry. The stable FH2/FH3 profile files should not need to be modified.

## How it works

For supported games Azimuth-DLAA:

- identifies the exact scene color/depth resolve,
- replays the game's velocity pass into full-resolution `R16G16_FLOAT` motion vectors,
- injects temporal camera jitter using the game profile's scene-depth rules,
- evaluates NVIDIA DLAA on the HDR scene image,
- restores the game's D3D12 state,
- and falls back to zero motion vectors with a history reset when the normal velocity pass stops in pause/photo states.

FH2 and FH3 intentionally keep separate profile details. For example, their motion-vector constant-buffer row layouts differ, and FH3 requires additional D32 depth handling.

## Logging and reports

The mod writes `azimuth_dlaa.log` next to the DLLs. When reporting an issue, include the log, Azimuth version, GPU, NVIDIA driver, resolution, game/DLC, and where the problem occurred.

## Building from source

The build produces two project DLLs:

- `dxgi.dll` - shared Azimuth/D3D12 hook and per-game integration
- `azimuth_ngx.dll` - shared NVIDIA NGX/DLSS bridge

The NGX bridge is built with MSVC, while the DXGI proxy is built with Zig/MinGW.

1. Obtain the NVIDIA DLSS SDK separately and place it in `third_party/DLSS`, or set `DLSS_SDK`.
2. From an x64 Visual Studio tools prompt, run `bridge\build_bridge.bat`.
3. Run `./build.sh` to build `dxgi.dll`.
4. Run `./build.sh release` to create a release ZIP after both project DLLs have been built.

The release target intentionally does **not** bundle `nvngx_dlss.dll`.

## Credits

Azimuth-DLAA is based on [FH2-FSR4](https://github.com/aranhoso/FH2-FSR4) by **aranhoso / Guilherme Fernandes Medeiros**. That project provided the original foundation for the Azimuth/D3D12 hooking, scene integration, motion-vector reconstruction, and temporal anti-aliasing path used by the FH2 implementation.

This mod is designed for [Azimuth](https://almost-there-games.itch.io/azimuth), an independent project by **Almost There Games**.

## Licenses

The Azimuth-DLAA source code is distributed under the MIT License. See `LICENSE`.

NVIDIA DLSS/NGX SDK components are governed by NVIDIA's own license. See `LICENSE-NVIDIA-DLSS.txt`.

NVIDIA SDK files are not part of this repository's MIT-licensed source.

Azimuth-DLAA is an unofficial community mod and is not affiliated with NVIDIA, Microsoft, Xbox, Playground Games, Turn 10 Studios, or Almost There Games.
