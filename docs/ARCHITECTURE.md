# MGET — Architecture and roadmap / Mimari

## v0.1: implemented modules
- `src/main.cpp`: UTF-16 Windows CLI entry point, argument parser, built-in providers, manifest execution, copy-mode tracking, update channel.
- `src/mini_json.hpp`: standalone UTF-8 JSON parser (no external dependency).
- `src/platform.hpp`: Win32 file picker, process launching, path helpers, registry, shortcut, WinHTTP HTTPS, SHA-256.
- `src/installer.cpp`: Win32 interactive single-EXE setup, directory selection, embedded payload writing, per-user PATH update.
- `src/installer.rc`: embeds the CLI EXE into the setup EXE.
- `scripts/build.ps1`: creates native CLI and one-file installer with LLVM-MinGW.
- `docs/`: CLI and manifest specifications, Turkish and English.

MGET default setup directory: `%LOCALAPPDATA%\Programs\MGET`.  
Tracked files: `%LOCALAPPDATA%\MGET\installed.tsv`.  
Update preferences: `%LOCALAPPDATA%\MGET\channel.txt`, `feed.txt`.  
Package downloads: directly to `--loc` or folder picker (**no persistent download cache**). Temporary `.part` files are written beside the final download and renamed when complete.

## Provider strategy / Sağlayıcı yapısı
1. Adapter interface with `search / resolve / download / install / update / remove`.
2. WinGet provider: invokes WinGet CLI (if installed).
3. Chocolatey provider: invokes choco CLI (if installed).
4. NPM provider: invokes npm within an explicit project directory.
5. Direct HTTPS provider: WinHTTP.
6. GitHub: public repository default-branch source ZIP; public release asset list with multi-selection. JSON handled in memory.
7. Planned: Google Drive links, MediaFire links, pip, Cargo, NuGet, Scoop, local registry, private LAN repository.

## Manifest lifecycle
Resolve package → optional `.mget` discovery → preview and consent → fetch and checksum → stage → install or copy → track → upgrade/remove. Current version implements a simplified form of this pipeline; full rollback is planned.

## Security backlog (must-have before stable 1.0)
- Pin updater public signing key; verify signed release manifest before trusting its checksum.
- HTTPS redirects with a checked hostname allowlist and bounded response size.
- Remove/reduce privileged actions; require explicit per-action confirmation.
- Transactional file installation with atomic moves, collision detection and rollback.
- Zip-slip protection and manifest detection inside archives.
- Manifests with explicit capability allowlists, no arbitrary hooks without sandboxing.
- Predictable package identity and normalized dependency graph.
- WinGet/Choco provider-aware uninstall and update reports.

## Roadmap
**0.1:** native CLI, single EXE setup, user PATH, WinGet/Chocolatey/npm adapters, optional manifest, manual update feed.  
**0.2:** reliable GitHub Releases API, dependency resolver, manifest archive discovery, fully tracked ownership and uninstall.  
**0.3:** authenticated artifact feeds, local/LAN package registry, portable app management, provider plugin API.  
**0.4:** signed releases, transactional updates and rollback, GUI installation wizard improvements.  
**1.0:** stable manifest contract, cross-provider tests and release signing.

No servers, official update URL, or online MGET package registry have been deployed yet. Examples are illustrative.
