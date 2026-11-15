# MGET Smoke Test Results / Hızlı Test Sonuçları

Test target: Windows development machine | Version: 0.1.0

## Verified / Doğrulanan
- C++20 single-executable CLI compile: PASS
- Embedded-resource single-file installer compile: PASS
- `mget --version`: PASS (0.1.0)
- `mget doctor`: PASS (WinGet present, npm present, Chocolatey absent)
- `mget inspect examples/demo/.mget`: PASS
- `mget install examples/demo --use-.mget --loc{...} --dry-run`: PASS
- Real local installation within `build/smoke`: PASS (2 files)
- `mget list`: PASS (2 tracked file records)
- `mget remove demo.package` with explicit confirmation: PASS (removed 2 tracked files)
- Rebuild after cleanup: PASS (no compiler warnings)

## Deliberately not tested / Bilerek çalıştırılmadı
- Installer v0.1.0-beta.2: console subsystem build PASS, embedded payload verification PASS, `N` cancel produced no files PASS, `Y` installed into a temporary test directory PASS, installed CLI started PASS. Tests used `--no-path-for-test`; actual HKCU PATH mutation was NOT performed on user's live profile.
- Third-party WinGet/Chocolatey/npm package installation: NOT RUN (avoids unintended software changes).
- Public update feed and `mget --update`: NOT TESTED (official release feed not configured).
- GitHub numbered display `[1] filename`: PASS. Ordered arbitrary selection `3-1-2`, `4-5-2`, `2-4-6-8` with `--dry-run`: PASS.
- GitHub HTML client (no REST API): verified on real public pages. Repository default branch source ZIP download: PASS. Release asset listing: PASS. `--select=2-4-6-8 --dry-run`: PASS on a release with 22 assets. Direct download of a small release asset: PASS. No persistent download cache; temporary test files removed: PASS.
- Google Drive / MediaFire landing-page downloads and redirects: NOT TESTED.

## Files
- `build/mget.exe`: 2,230,272 bytes
- `build/MGET-Setup.exe`: 5,443,072 bytes; embeds `mget.exe`, `MGET-Uninstall.exe`, and the black-M icon (beta.2).

The single-file setup embeds the CLI binary, defaults to `%LOCALAPPDATA%\Programs\MGET`, and updates `HKCU\Environment\Path` after installation. A new terminal is required to see the new PATH.

## Beta.2 — terminal uninstaller and logo
- Single-file console installer built with CLI + embedded uninstaller: PASS
- MGET icon generated as transparent black `M` (SVG, PNG, ICO) and embedded in EXEs: PASS
- Setup `N` cancellation leaves test directory absent: PASS
- Setup `Y` copies `mget.exe` and `MGET-Uninstall.exe`: PASS
- Installed CLI launched and returned `MGET 0.1.0`: PASS
- Uninstaller `N` cancellation leaves installed files untouched: PASS
- Uninstaller internal cleanup from temporary copy removes both binaries and an empty test directory: PASS
- Actual PATH/Installed Apps registration and interactive child-helper UI: NOT live-tested on main user profile
