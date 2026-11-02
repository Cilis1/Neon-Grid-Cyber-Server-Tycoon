# The Bootstrap Installer (`CyberServerTycoonSetup.exe`)

A small (< 5 MB) standalone Windows executable built from `installer/`. It
downloads the full game from the latest GitHub release, installs it, creates a
desktop shortcut with the game icon, and launches the game. Re-running it
updates the game.

## Flow

1. **Install root**: `%LOCALAPPDATA%\NeonGrid\` (override with `--install-dir`).
   Game lives in `%LOCALAPPDATA%\NeonGrid\game\`.
2. **Manifest**: fetches `https://github.com/<owner>/<repo>/releases/latest/download/version.json`
   (libcurl, 10 s timeout; override with `--url`):

   ```json
   { "version": "1.0.0", "game_zip_url": "https://…/game.zip", "sha256": "abc…", "changelog": "…" }
   ```

3. **Version check**: reads `%LOCALAPPDATA%\NeonGrid\version.txt`. Downloads only
   if the remote version is newer or `NeonGrid.exe` is missing.
4. **Download**: `game.zip` → `%TEMP%\neongrid_game.zip`, console progress
   (`--silent` suppresses output).
5. **Verify**: SHA-256 of the zip compared to `sha256` in the manifest; one
   retry on mismatch, then abort with a `MessageBox` and `%LOCALAPPDATA%\NeonGrid\installer.log`.
6. **Extract**: miniz (`mz_zip_reader_*`) into the game dir, overwriting files,
   with zip-slip protection. Writes `version.txt`.
7. **Shortcut**: `Neon Grid - Cyber Server Tycoon.lnk` on the Desktop via COM:

   ```cpp
   CoCreateInstance(CLSID_ShellLink, …, IID_IShellLinkW, …);
   psl->SetPath(game_exe); psl->SetWorkingDirectory(game_dir);
   psl->SetIconLocation(icon, 0); psl->SetDescription("Neon Grid: Cyber Server Tycoon");
   psl->QueryInterface(IID_IPersistFile, …)->Save(link_path, TRUE);
   ```

   (links `ole32`, `uuid`, `shell32`; implemented in `installer/bootstrap.cpp`
   via `utils/WindowsShortcut.cpp`).
8. **Launch**: `NeonGrid.exe --installed` via `CreateProcessW`.

## CLI flags

```
CyberServerTycoonSetup.exe [--silent] [--no-launch]
                           [--url <version.json URL>] [--repo <owner/repo>]
                           [--install-dir <dir>]
```

## Testing locally without GitHub

Serve a fake release over HTTP and point the bootstrap at it:

```bash
# build a fake release payload
mkdir -p /tmp/http/game && cp NeonGrid.exe SDL2.dll … /tmp/http/game/
python scripts/pack_assets.py /tmp/http/game /tmp/http/game.zip
SHA=$(sha256sum /tmp/http/game.zip | cut -d' ' -f1)
cat > /tmp/http/version.json <<EOF
{ "version": "9.9.9", "game_zip_url": "http://127.0.0.1:8000/game.zip", "sha256": "$SHA" }
EOF
cd /tmp/http && python -m http.server 8000 &
CyberServerTycoonSetup.exe --url http://127.0.0.1:8000/version.json --install-dir %TEMP%\NeonGridTest --no-launch
```

On Linux the same flow works (shortcut creation is a no-op off-Windows):
`./CyberServerTycoonSetup --url http://127.0.0.1:8000/version.json --install-dir /tmp/neongrid-test --no-launch`.

## Uninstall

Delete `%LOCALAPPDATA%\NeonGrid\` and the desktop shortcut. That's everything.
