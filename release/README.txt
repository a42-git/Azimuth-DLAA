Azimuth-DLAA 0.2.0 alpha - unified FH2/FH3 public-test build

SUPPORTED
  Forza Horizon 2                         Azimuth 1.0.3 / 1.0.4
  Storm Island                            Azimuth 1.0.3 / 1.0.4
  Forza Horizon 2 Presents Fast & Furious Azimuth 1.0.3 / 1.0.4
  Forza Horizon 3                         Azimuth 1.0.4

Forza Motorsport 5 and Forza Motorsport 6 are not supported by this build yet.

INSTALLATION
1. Copy dxgi.dll and azimuth_ngx.dll next to azimuth.exe.
2. Provide nvngx_dlss.dll from an official NVIDIA DLSS SDK/runtime source.
3. Turn anti-aliasing off in Azimuth.
4. Launch a supported game.

The correct game profile is selected automatically.

CONTROL
  F11  toggle DLAA on/off

PHOTO MODE
DLAA is supported in photo mode. When the game stops producing its normal velocity pass,
Azimuth-DLAA uses zero motion vectors with a history reset. If a completely stationary
photo frame does not visibly update after F11, move the camera slightly to force a redraw.

LOG
  azimuth_dlaa.log

If reporting an issue, include the log plus your Azimuth version, GPU, NVIDIA driver,
resolution, game/DLC and where the issue happened.

FILES
  dxgi.dll        shared D3D12/game integration
  azimuth_ngx.dll NVIDIA NGX bridge

nvngx_dlss.dll is intentionally not included in the GitHub release package.

CREDITS
Azimuth-DLAA is based on FH2-FSR4 by aranhoso / Guilherme Fernandes Medeiros:
https://github.com/aranhoso/FH2-FSR4

Designed for Azimuth by Almost There Games:
https://almost-there-games.itch.io/azimuth

Unofficial mod. Not affiliated with NVIDIA, Microsoft, Xbox, Playground Games,
Turn 10 Studios or Almost There Games.
