# MGET — Frequently Asked Questions

**“mget is not recognized”** — Close your old terminals and open a new PowerShell/CMD window. Test the default installation with `& "$env:LOCALAPPDATA\Programs\MGET\mget.exe" --version`.

**“Chocolatey not installed”** — It is optional; check `mget doctor`. Choose `--source=winget` for WinGet packages.

**How do I choose the destination?** — Use `--loc="C:\Apps\Demo"` for local copies. Without it, Windows opens a folder picker. External EXE/MSI, WinGet and Chocolatey installers choose their own installation location.

**“.mget required but not found”** — Provide a package-root `.mget` manifest or remove `--use-.mget`.

**“Link returned an HTML page”** — Direct file links are needed, except MGET now handles GitHub repository/release pages. Generic Google Drive/MediaFire landing pages are not yet supported.

**GitHub URL behavior?** — A repository root URL downloads the default-branch source ZIP; a `/releases` URL lists attached artifacts. `--select=1-2` selects files 1 and 2; `0` has no special meaning. [GitHub guide](GITHUB-EN.md).

**Does it create a cache?** — No persistent download cache. Files download directly to the chosen directory. A transient `.part` file may appear during an active transfer.

**Pull vs install?** — For ordinary direct HTTPS files, `pull` only downloads; `install` may launch an installer or extract archives. For GitHub repository/release URLs, both download without auto-execution.

**Does list show all apps?** — No, only directly tracked MGET file copies.

**Does remove uninstall WinGet/Chocolatey software or undo registry entries?** — No. Only recorded copied files are removed.

**Why does self-update need configuration?** — An official update feed has not yet been deployed. Signature authentication is not implemented; use only an update feed you own and trust.

[Quick Start](QUICKSTART-EN.md) · [Cheat Sheet](CHEATSHEET-EN.md)
