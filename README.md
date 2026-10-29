# Azimuth-DLAA

NVIDIA DLAA support for Forza games running through [Azimuth](https://almost-there-games.itch.io/azimuth).

> **Current support:** Forza Horizon 2 is the first supported game. Support for other Azimuth titles is planned, but they are not supported by the current public build yet.

Azimuth-DLAA is an unofficial community mod and is not affiliated with NVIDIA, Microsoft, Xbox, Playground Games, Turn 10 Studios, or Almost There Games.

## Supported games

| Game | Status |
| --- | --- |
| Forza Horizon 2 | ✅ Supported |
| Storm Island | ✅ Supported |
| Forza Horizon 2 Presents Fast & Furious | ✅ Supported |
| Forza Horizon 3 | 🚧 Planned / not working yet |
| Forza Motorsport 5 | ⚠️ Untested |
| Forza Motorsport 6 | 🚧 Planned |

## Current public-test target

- **Forza Horizon 2**
- **Azimuth 1.0.3**
- NVIDIA GeForce RTX GPU
- Native-resolution NVIDIA DLAA

Azimuth 1.0.4 was released after the current FH2 build was prepared and is still being validated.

## Installation

1. Install [Azimuth](https://almost-there-games.itch.io/azimuth) and make sure Forza Horizon 2 launches normally first.
2. Download the latest Azimuth-DLAA release from this repository.
3. Copy the included mod DLLs next to `azimuth.exe`.
4. Provide the required NVIDIA DLSS runtime (`nvngx_dlss.dll`) from an official NVIDIA DLSS SDK/runtime source if it is not included with the release.
5. Set anti-aliasing to **Off** in Azimuth.
6. Launch Forza Horizon 2.

To uninstall, remove the Azimuth-DLAA DLLs from the Azimuth folder.

## Controls

- **F11** — toggle DLAA on/off.

F11 works in free roam, car meets and photo mode.

In photo mode, if the game is not producing a new frame while the camera is completely stationary, the visible image may not update immediately after toggling DLAA. Move the photo camera slightly to force a redraw.

## What it does

For Forza Horizon 2, Azimuth-DLAA hooks Azimuth's D3D12 rendering path and replaces the game's MSAA path with NVIDIA DLAA.

The FH2 integration:

- identifies the game's scene colour/depth resolve,
- reconstructs full-resolution motion vectors from FH2's motion-blur velocity pass,
- injects sub-pixel camera jitter,
- runs NVIDIA DLAA on the HDR scene image,
- restores the result before later post-processing,
- and uses a zero-motion-vector/history-reset fallback when FH2 stops producing its normal velocity pass in pause or photo mode.

## Optional NVIDIA Profile Inspector tweaks

These settings are **not required** for Azimuth-DLAA to work.

Based on my own testing, DLAA presets can also be forced by adding `azimuth.exe` to NVIDIA Profile Inspector and changing the DLSS preset there.

At 4K, I found:

- **Preset E** — good balance
- **Preset K** — good balance
- **Preset M** — much heavier GPU usage and generally overkill for my setup

Results may vary depending on GPU, resolution and driver version.

I also saw a substantial performance improvement on my system by forcing **Resizable BAR / ReBAR** for the Azimuth profile. This is an optional driver-profile tweak and may not behave the same on every system.

## Logging and issue reports

A small troubleshooting log is written to:

`fh2_dlss.log`

If you run into a problem, feel free to open an issue or report it with as much detail as possible.

Please include:

- `fh2_dlss.log`
- Azimuth version
- GPU model
- NVIDIA driver version
- resolution
- which game/DLC was running
- where the problem happened: gameplay, menu, pause, photo mode, etc.
- whether DLAA was enabled or disabled with F11
- screenshots or video if the issue is visual

Suggestions and testing feedback are welcome.

## Building from source

The FH2 implementation builds two DLLs:

- `dxgi.dll` — the Azimuth/D3D12 proxy and game integration
- `fh2_ngx.dll` — the NVIDIA NGX/DLSS bridge

The NGX bridge is built with MSVC, while the DXGI proxy is built with Zig/MinGW.

1. Obtain the NVIDIA DLSS SDK separately and place it in `third_party/DLSS`, or set `DLSS_SDK`.
2. Build `bridge/build_bridge.bat` from an x64 Visual Studio tools prompt.
3. Build the proxy with `./build.sh`.

NVIDIA SDK files are not part of this repository's MIT-licensed source.

## Project direction

FH2 is currently the only supported game.

Planned work includes investigating support for:

- Forza Horizon 3
- Forza Motorsport 5
- Forza Motorsport 6
- newer Azimuth versions

Compatibility should not be assumed until a game/version is explicitly listed as supported.

## Credits

Azimuth-DLAA is based on [FH2-FSR4](https://github.com/aranhoso/FH2-FSR4) by **aranhoso / Guilherme Fernandes Medeiros**.

That project provided the original foundation for the Azimuth/D3D12 hooking, scene integration, motion-vector reconstruction, and temporal anti-aliasing path used by the FH2 implementation.

This mod is designed for [Azimuth](https://almost-there-games.itch.io/azimuth), an independent project by **Almost There Games** that runs supported Xbox One Forza titles through a translation layer.

## Licenses

The Azimuth-DLAA source code is distributed under the MIT License. See `LICENSE`.

NVIDIA DLSS/NGX SDK components are governed by NVIDIA's own license. See `LICENSE-NVIDIA-DLSS.txt`.

The upstream FH2-FSR4 copyright notice is retained as required by its MIT license.
