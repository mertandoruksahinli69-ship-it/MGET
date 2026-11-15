# MGET v0.1.0-beta.1 — Release Notes

First Windows beta / İlk Windows beta sürümü.

## Included / Dahil edilenler

- Standalone C++20 Windows CLI (`mget.exe`)
- Single-file Windows installer (`MGET-Setup.exe`), per-user PATH configuration
- WinGet and optional Chocolatey search / delegated installations
- npm project dependency delegation
- Local file and folder installs, optional JSON `.mget` manifest
- Manifest copy mappings, registry string entries and Start Menu shortcuts
- Direct HTTPS downloads with no persistent download cache
- GitHub repository source ZIP fetching and GitHub release asset list/selection (experimental)
- Separate `--update` command with configurable self-update feed
- Turkish and English documentation

## Important / Önemli

**Beta, not a stable release.** Do not install untrusted programs.

GitHub release listing requires the GitHub public API. During local testing, unauthenticated requests returned **HTTP 403 and a rate-limit remaining value of zero**. Runtime GitHub download flow is not yet verified end-to-end. No GitHub API credentials or authentication are bundled.

Installer behavior, WinGet/Chocolatey installation, real self-update replacement, and third-party package installation have not undergone complete end-to-end testing. Self-update manifest verification is **not cryptographically authenticated**, so do not configure unknown feeds.

`mget remove` only reverses directly tracked file copies, not external package managers or registry modifications.

No private information or user-specific absolute directory paths are included in the public documentation.

## How to start / Başlangıç

1. Download `MGET-Setup.exe`.
2. Run it and select the default or custom installation folder.
3. Open a new CMD or PowerShell terminal.
4. Run `mget --version` and `mget doctor`.

See `docs/START-HERE.md`, `docs/QUICKSTART-TR.md` and `docs/QUICKSTART-EN.md`.
