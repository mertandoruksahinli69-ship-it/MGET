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
- GUI setup installation and PATH registry mutation: NOT RUN on user's live desktop; user may run `build/MGET-Setup.exe` when ready.
- Third-party WinGet/Chocolatey/npm package installation: NOT RUN (avoids unintended software changes).
- Public update feed and `mget --update`: NOT TESTED (official release feed not configured).
- GitHub API release listing: NOT VERIFIED end-to-end; public dry-run returned HTTP 403 (possibly rate limiting or network policy). GitHub integration compiles but needs an online test in an environment with API access.
- Google Drive / MediaFire landing-page downloads and redirects: NOT TESTED.

## Files
- `build/mget.exe`: 2,230,272 bytes
- `build/MGET-Setup.exe`: 4,188,160 bytes, includes embedded `mget.exe`.

The single-file setup embeds the CLI binary, defaults to `%LOCALAPPDATA%\Programs\MGET`, and updates `HKCU\Environment\Path` after installation. A new terminal is required to see the new PATH.
