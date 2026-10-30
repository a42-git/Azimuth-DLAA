# Adding another Azimuth game

Do not clone `hooks.c` or `dlss.c` for a new game. Keep discovery/diagnostic code on a separate development branch and only promote validated constants/behavior into a new game profile.

## Discovery workflow

1. Start from a scan/trace branch, not the public build.
2. Identify the game's motion-vector/velocity pass and real full-resolution scene resolve.
3. Verify execution order, viewport size, RTV/DSV formats and cadence at runtime.
4. Reconstruct full-resolution motion vectors and validate that replay independently.
5. Enable DLAA only after MV replay and resolve timing are stable.
6. Test gameplay, menus/transitions, photo/pause states and resolution changes.
7. Remove scanners/dumps again before merging support into the main branch.

## Stable-profile workflow

Create `src/games/<game>.c` containing:

- exact motion-vector shader hash(es),
- exact resolve shader hash(es),
- the game's MV reconstruction HLSL,
- MV output resolution scale,
- expected resolve color/depth formats,
- depth formats that identify jitterable scene rendering,
- any small compatibility flags already represented by `GameProfile`.

Then add one `extern` and one entry to `src/game_profiles.c`, and add the source file to `build.sh`.

If a new game needs behavior that cannot be expressed by `GameProfile`, add the smallest generic capability to the profile interface rather than putting `if (game == ...)` checks throughout `hooks.c`.

## Rules for keeping the source clean

- `src/hooks.c`: shared interception/render orchestration only.
- `src/dlss.c`: shared NGX/DLAA behavior only.
- `src/games/*.c`: game hashes, reconstruction shader and game-specific flags only.
- No shader dumping/scanning in the stable branch.
- Never change an already-validated profile while porting a new game unless testing proves it is required.
- Prefer exact shader hashes over structural fallbacks in public builds.
- Keep the one-process profile lock: a process can select one game profile and never switch later.
