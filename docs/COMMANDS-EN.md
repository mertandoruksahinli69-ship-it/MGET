# MGET — CLI Reference (EN)

Version: **0.1.0 developer preview**.

| Command | Purpose |
|---|---|
| `mget help` | Show help |
| `mget doctor` | Check provider availability and update configuration |
| `mget --version` | Show CLI version |
| `mget search <name>` | Search WinGet and Chocolatey if installed |
| `mget search <name> --source=winget` | Search WinGet |
| `mget search <name> --source=choco` | Search Chocolatey |
| `mget search <name> --source=npm` | Search npm |
| `mget install <name>` | Search providers and prompt for selection |
| `mget install <name> --source=winget` | Delegate installation to WinGet |
| `mget install <name> --source=choco` | Delegate installation to Chocolatey |
| `mget install npm:<name> --project=<folder>` | Install npm package into an existing project |
| `mget install <https-url>` | Download and install/copy a direct HTTPS file |
| `mget install <local-path>` | Install a local file or directory |
| `mget install <path> --use-.mget` | Require the optional `.mget` manifest |
| `mget install <path> --manifest=<file>` | Supply an explicit manifest |
| `mget init [folder]` | Generate a new `.mget` manifest |
| `mget inspect <manifest>` | Display manifest summary |
| `mget pull <https-url>` | Download directly to destination without installing; print SHA-256 |
| `mget pull https://github.com/OWNER/REPO` | Download default-branch source ZIP |
| `mget pull https://github.com/OWNER/REPO/releases --select=1-2` | Download assets numbered 1 and 2 |
| `mget pull https://github.com/OWNER/REPO/releases --all` | Download all release assets |
| `mget list` | List MGET-tracked files |
| `mget remove <package>` | Remove only tracked copied files after confirmation |
| `mget upgrade [package]` | Delegate package update to WinGet |
| `mget upgrade <package> --source=choco` | Delegate to Chocolatey |
| `mget deploy <folder> --target=<folder>` | Local deployment |
| `mget channel <stable\|beta\|nightly>` | Choose update channel |
| `mget channel stable --feed=<https-url>` | Configure release feed |
| `mget --update` | Update the MGET program itself |
| `mget --update --check` | Verify/check/download update without applying |

## Installation paths
```powershell
mget install ".\ExamplePackage" "--loc{C:\Apps\Example}"
mget install ".\ExamplePackage" --loc="C:\Apps\Example"
mget install ".\ExamplePackage" "--loc{kullanıcıadı/İndirilenler}"
```
Local installs open the Windows folder picker when `--loc` is omitted. Third-party WinGet/Chocolatey installers retain control of their own destination. EXE/MSI launches request confirmation. ZIP archives are extracted; other single files are copied.

## Manifest
The optional `.mget` JSON file controls installation mappings, an installer mode, registry string values and Start Menu shortcuts. `--use-.mget` requires it. `--manifest=<path>` selects a specific file. No manifest is needed for simple installers or copying. Sensitive settings require confirmation. Full rollback and general hook scripting are **not** in v0.1.

## Update protocol
`mget channel stable --feed=https://example.org/mget/releases/` configures the release base URL. MGET fetches `stable.json` (or `beta.json` / `nightly.json`) with:
```json
{"version":"0.1.1","url":"https://example.org/mget/releases/mget-0.1.1.exe","sha256":"<64-character lowercase SHA-256 hex>"}
```
After downloading, the binary hash is checked. The experimental updater performs a self-replacement from a separate temporary helper executable. Never point the feed to an untrusted source: the index is **not digitally signed** in v0.1.

## Single-file setup
`MGET-Setup.exe` contains `mget.exe` as an embedded Windows resource. By default it installs to `%LOCALAPPDATA%\Programs\MGET`; an alternate folder can be chosen. Setup adds that directory once to **HKCU user PATH**, broadcasts an environment-change notification, and prompts users to open a **new** terminal. No admin rights are required for the default folder.

## Known limitations
- Not all Google Drive/MediaFire sharing pages expose direct download URLs; interactive downloads and login flows are not supported in v0.1.
- npm requires Node.js; Chocolatey requires Chocolatey to already be installed.
- `mget remove` does not uninstall WinGet/Chocolatey applications or reverse registry changes.
- Authentic release signing, full dependency resolution, comprehensive backup/rollback, remote deployment and portable-installed-program detection are planned.
