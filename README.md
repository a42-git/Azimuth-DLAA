# Azimuth-DLAA

NVIDIA DLAA support for games running through [Azimuth](https://almost-there-games.itch.io/azimuth).

> **Current support:** Forza Horizon 2 is the first supported game. Support for other Azimuth titles is planned, but they are not supported by the current public build yet.

Azimuth-DLAA is an unofficial community mod and is not affiliated with NVIDIA, Microsoft, Xbox, Playground Games, Turn 10 Studios, or Almost There Games.

## Supported games

| Game | Status |
| --- | --- |
| Forza Horizon 2 | ✅ Supported |
| Storm Island | ✅ Supported |
| Forza Horizon 2 Presents Fast & Furious | ✅ Supported |
| Forza Horizon 3 | 🚧 Planned / not working yet |
| Forza Motorsport 5 | 🚧 Planned |
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

F11 also works in photo mode. If the game is not producing a new frame while the photo camera is completely stationary, the visible image may not update until the next redraw.

## What it does

For Forza Horizon 2, Azimuth-DLAA hooks Azimuth's D3D12 rendering path and replaces the game's MSAA path with NVIDIA DLAA.

The FH2 integration:

- identifies the game's scene colour/depth resolve,
- reconstructs full-resolution motion vectors from FH2's motion-blur velocity pass,
- injects sub-pixel camera jitter,
- runs NVIDIA DLAA on the HDR scene image,
- restores the result before later post-processing,
- and uses a zero-motion-vector/history-reset fallback when FH2 stops producing its normal velocity pass in pause or photo mode.

## Logging and issue reports

A small troubleshooting log is written to:

`fh2_dlss.log`

When reporting a problem, please include:

- `fh2_dlss.log`
- Azimuth version
- GPU model
- resolution
- which game/DLC was running
- where the problem happened (gameplay, menu, pause, photo mode, etc.)

## Building from source

The FH2 implementation builds two DLLs:

- `dxgi.dll` — the Azimuth/D3D12 proxy and game integration
- `fh2_ngx.dll` — the NVIDIA NGX/DLSS bridge

The NGX bridge is built with MSVC, while the DXGI proxy is built with Zig/MinGW.

1. Obtain the NVIDIA DLSS SDK separately and place it in `third_party/DLSS`, or set `DLSS_SDK`.
2. Build `bridge/build_bridge.bat` from an x64 Visual Studio tools prompt.
3. Build the proxy with `./build.sh`.

NVIDIA SDK files are not part of this repository's MIT-licensed source.

## Credits

Azimuth-DLAA is based on [FH2-FSR4](https://github.com/aranhoso/FH2-FSR4) by **aranhoso / Guilherme Fernandes Medeiros**.

That project provided the original foundation for the Azimuth/D3D12 hooking, scene integration, motion-vector reconstruction, and temporal anti-aliasing path used by the FH2 implementation.

This mod is designed for [Azimuth](https://almost-there-games.itch.io/azimuth), an independent project by **Almost There Games** that runs supported Xbox One Forza titles on Windows and Linux through a translation layer.

## Licenses

The Azimuth-DLAA source code is distributed under the MIT License. See `LICENSE`.

NVIDIA DLSS/NGX SDK components are governed by NVIDIA's own license. See `LICENSE-NVIDIA-DLSS.txt`.

The upstream FH2-FSR4 copyright notice is retained as required by its MIT license.
