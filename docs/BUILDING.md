# Building Neon Grid: Cyber Server Tycoon

## Requirements

- Windows 10/11, **Visual Studio 2022** (MSVC, C++20), **CMake ≥ 3.24**
- Python 3 with **Pillow** (`pip install pillow`) — only for icon generation
- Git (for FetchContent)

No other manual dependency installs: SDL2, SDL2_image/ttf/mixer, Dear ImGui,
Lua 5.4, sol2, nlohmann/json, spdlog, libcurl, miniz and Catch2 are all fetched
and built automatically by CMake `FetchContent`.

## Quick start (Windows / MSVC)

```bat
python scripts/generate_icon.py     # generates assets/icon/icon.ico + .png (idempotent)
python scripts/download_fonts.py    # optional; game falls back to the default font
cmake --preset windows-msvc
cmake --build --preset windows-msvc-release
ctest --test-dir build\windows-msvc -C Release --output-on-failure
build\windows-msvc\Release\NeonGrid.exe
build\windows-msvc\Release\CyberServerTycoonSetup.exe   # bootstrap installer
```

Presets available (see `CMakePresets.json`):

| Preset | Purpose |
| --- | --- |
| `windows-msvc` / `windows-msvc-release` | Release build with VS2022 x64 |
| `windows-msvc-debug` | Debug build |
| `linux-dev` | Linux: core logic + tests only (no SDL GUI), for CI/dev on Linux |

## What gets built

| Target | Output | Notes |
| --- | --- | --- |
| `neongrid_core` | static lib | all game logic, no SDL/ImGui — unit-tested |
| `NeonGrid` | `NeonGrid.exe` | the game; icon embedded via `src/resources.rc` |
| `CyberServerTycoonSetup` | `CyberServerTycoonSetup.exe` | bootstrap installer, static curl+miniz, icon embedded |
| `test_*` | test exes | Catch2; run with `ctest` |

## Packaging the game zip (what CI does)

```bat
cmake --install build\windows-msvc --prefix build\staging --config Release
python scripts\pack_assets.py build\staging build\game.zip
```

`game.zip` contains `NeonGrid.exe`, the SDL2 DLLs, `assets/`, `data/`, and
`scripts/lua/`.

## CMake options

| Option | Default | Meaning |
| --- | --- | --- |
| `NEON_BUILD_GUI` | ON | build the SDL2/ImGui game exe |
| `NEON_BUILD_INSTALLER` | ON | build the bootstrap installer |
| `NEON_BUILD_TESTS` | ON | build Catch2 tests |
| `NEON_USE_CURL` | ON | libcurl networking (installer download / version checker) |
| `NEON_USE_LUA` | ON | Lua 5.4 scripting via sol2 (falls back to C++ effects if OFF) |

Example headless logic-only build on Linux:

```bash
cmake -S . -B build/linux -G Ninja -DNEON_BUILD_GUI=OFF -DNEON_BUILD_TESTS=ON
cmake --build build/linux
ctest --test-dir build/linux
```

## Troubleshooting

- **`icon.ico` missing / rc error**: run `python scripts/generate_icon.py`
  (CMake also auto-generates it via a custom command when Python3 + the file are
  available and the icon is absent).
- **FetchContent is slow the first time**: it clones SDL2/imgui/curl/etc. into
  `build/<preset>/_deps`. CI caches this directory.
- **SDL2 satellite libs fail on submodules**: we deliberately fetch only
  github-hosted submodules (`external/freetype` for ttf); image codecs use the
  built-in STB backend, so no googlesource submodules are needed.
- **curl build errors on Windows**: we force `CURL_USE_SCHANNEL=ON`,
  `HTTP_ONLY=ON`, static lib. If your environment has a system libcurl, CMake
  uses it instead (`find_package(CURL)` first).
- **Fonts missing**: run `python scripts/download_fonts.py`; the game falls back
  to ImGui's default font if the TTFs are absent.
- **Game can't find `data/`**: run the exe from the install/staging dir, or pass
  `--data <path-to-data-dir>`.
- **`--reset`**: deletes the save and starts fresh (useful for testing).

## MinGW

MinGW-w64 with GCC ≥ 12 can build the core + tests
(`-DNEON_BUILD_GUI=OFF`). The full SDL2 GUI and the Windows COM shortcut code
target MSVC; use the `windows-msvc` preset for the real game.
