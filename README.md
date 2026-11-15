# MGET — Modern Get

Native Windows C++20 package fetcher and installer.  
**Project version:** 0.1.0 (developer preview)  
**Distribution:** one-file installer `build\MGET-Setup.exe`; installs `mget.exe` and adds its directory to user PATH.

## Türkçe
MGET Windows'ta WinGet, isteğe bağlı Chocolatey, NPM ve doğrudan HTTPS indirmelerini tek CLI'de toplar. Yerel paketler için isteğe bağlı JSON biçimli `.mget` manifesti destekler.

**Kurulum:**
1. `build\MGET-Setup.exe` dosyasını aç.
2. Varsayılan kurulum dizinini onayla veya kendi klasörünü seç.
3. Kurulum `mget.exe` dosyasını yazıp kullanıcı PATH'ini günceller.
4. **Yeni bir** PowerShell/CMD aç ve `mget --version` yaz.

**Derleme:** `powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1`

**Komut örnekleri:**
```powershell
mget doctor
mget search vscode
mget install Microsoft.VisualStudioCode --source=winget
mget install npm:lodash --project="C:\Projeler\Web"
mget install "C:\Paketler\Demo" "--loc{C:\Apps\Demo}" --dry-run
mget pull https://example.org/archive.zip "--loc{C:\Downloads}"
mget inspect "C:\Paketler\Demo\.mget"
mget channel stable --feed=https://example.org/releases/
mget --update --check
```

**Dokümanlar:** [Buradan başla](docs/START-HERE.md), [TR hızlı başlangıç](docs/QUICKSTART-TR.md), [GitHub (TR)](docs/GITHUB-TR.md), [Komut tablosu (TR)](docs/CHEATSHEET-TR.md), [.mget (TR)](docs/MANIFEST-TR.md).

## English
MGET integrates WinGet, optional Chocolatey, npm project dependencies, local package copying, optional `.mget` JSON manifests, HTTPS downloads and update channels into a native Windows CLI. Its self-contained setup EXE embeds the CLI, installs it to the user profile, and updates the **user PATH**.

**Install:** Run `build\MGET-Setup.exe` → choose default or custom folder → open a **new** terminal → `mget doctor`.

**Build:** `powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1`

**Documentation:** [Start Here](docs/START-HERE.md), [Quick Start EN](docs/QUICKSTART-EN.md), [GitHub EN](docs/GITHUB-EN.md), [Command Cheat Sheet](docs/CHEATSHEET-EN.md), [Manifest EN](docs/MANIFEST-EN.md), [Architecture](docs/ARCHITECTURE.md)

## Feature status
| Function | Status |
|---|---|
| Single-file installer, per-user PATH, default/custom folder | Implemented |
| Standalone C++ CLI and Windows directory picker | Implemented |
| WinGet/Chocolatey search and delegated install | Implemented (provider must exist) |
| npm project install | Implemented (Node.js must exist) |
| Generic HTTPS binary downloads | Implemented; direct links only |
| Optional `.mget` copy-mode JSON mapping | Implemented |
| Manifest registry REG_SZ & Start Menu shortcuts | Implemented (confirmation required) |
| `mget --update` custom feed, hash verification, executable replacement | Developer preview; feed must be configured |
| `mget remove` | Only directly tracked file copies |
| GitHub source ZIP and interactive release assets | Implemented (public repository URLs; network validation pending) |
| Download directly to chosen folder; no persistent package cache | Implemented |
| Google Drive / MediaFire browser-page parsing and authentication | Planned |
| Verified signing/trust store, comprehensive rollback | Planned |
| Multiple deployment adapters, full NPM lifecycle | Planned |

**Security:** Only install trusted packages. Hash verification checks integrity relative to release metadata; it does **not** authenticate an untrusted update feed. Production auto-updates require signed manifests and trusted public keys.
