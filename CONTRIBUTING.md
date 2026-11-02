# Contributing

Thanks for helping build the grid!

## Workflow

1. Fork the repo and create a feature branch from `main`.
2. Keep changes focused; add tests for new systems (`tests/`, Catch2 v3).
3. Run `cmake --preset windows-msvc && cmake --build --preset windows-msvc-release`
   and `ctest --test-dir build/windows-msvc -C Release` before opening a PR.
4. CI (`.github/workflows/build-release.yml`) builds and tests every PR.

## Code style

- C++20, 2-space indent, `.clang-format` provided (LLVM base, 120 cols).
- No warnings with `/W3` on MSVC (or `-Wall -Wextra -Wpedantic` on GCC/Clang).
- Game logic lives in `neongrid_core` (no SDL/ImGui) so it stays unit-testable;
  UI lives in `src/ui/` and drives the game only through `core::Game`.
- All game content is data-driven JSON in `data/` — add content there instead of
  hard-coding. Lua hooks live in `scripts/lua/`.

## Adding content

- New tech node: add to `data/tech_tree.json` (see `docs/JSON_SCHEMAS.md`).
- New hardware: `data/hardware.json`. New program: `data/netrun_programs.json`
  (+ optional `scripts/lua/programs/<name>.lua`). New random event:
  `data/events.json` (+ optional `scripts/lua/events/<name>.lua`).
- After changing data, run the test suite — catalog validation happens at load.

## Releases

Push a tag `vX.Y.Z` on `main`; CI builds, packages `game.zip`, writes
`version.json` with the SHA-256, and publishes a GitHub Release with
`CyberServerTycoonSetup.exe`, `game.zip`, and `version.json`.
