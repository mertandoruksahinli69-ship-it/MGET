# MGET v0.1.0-beta.2 — Release Notes

## Türkçe

**Yeni:** Tek dosyalı `MGET-Setup.exe` artık Windows terminalinde açılır; grafik pencere göstermez.

Kurulum akışı:

1. `Install MGET now? [Y/N]` — **Y** kuruluma devam eder; **N** hiçbir değişiklik yapmadan iptal eder.
2. `Use default installation folder? [Y/N]` — **Y** kullanıcı profiline kurar; **N** farklı klasör yolu girmeyi sağlar.
3. Kurulumdan sonra MGET klasörü **kullanıcı PATH** değişkenine eklenir. Yeni terminal açınca `mget` çalışır.
4. Terminal üzerinden alternatif kurulum yolu: `MGET-Setup.exe --dir="D:\Apps\MGET"`.

EXE'ye çift tıklandığında terminal kurulum sonrası Enter bekler. CLI uygulaması `mget.exe` ve yeni terminal tabanlı `MGET-Uninstall.exe` kurulum dosyasının içine gömülüdür. Kaldırıcı `Y/N` onayıyla yalnız MGET'in kurulu dosyalarını, kayıtlı PATH satırını ve Windows uygulama kaydını temizler; kullanıcı verilerini silmez.

**Logo:** Şeffaf zemin üzerinde sadece siyah `M`; SVG, PNG ve uygulama ICO simgesi.

**Test:** CLI ve kurulum derlemesi başarılı; gömülü `mget.exe` ve `MGET-Uninstall.exe` doğrulandı; `N` ile iptal edildiğinde dosya oluşmadığı, `Y` ile yerel test kurulumunda CLI'nin açıldığı doğrulandı. Kaldırıcının `N` iptali ve test klasörünü temizlemesi doğrulandı. Gerçek kullanıcı PATH kaydı değiştirilmedi.

## English

**Changed:** The one-file `MGET-Setup.exe` now launches a **console** instead of a graphical wizard.

- `Install MGET now? [Y/N]`: **Y** continues; **N** cancels without modifying files.
- `Use default installation folder? [Y/N]`: **Y** uses the per-user default; **N** lets you type a custom path.
- The installed folder is automatically added to your **user PATH** (HKCU), unless a private local test flag is specified.
- Optional custom destination: `MGET-Setup.exe --dir="D:\Apps\MGET"`.
- When launched interactively, press **Enter** to close the terminal after setup.
- **New terminal uninstaller:** `MGET-Uninstall.exe` is embedded in the one-file setup and installed beside the CLI. It offers `Y/N` confirmation and removes only its own binaries plus the matching PATH and Windows Apps registration.
- **New icon:** a solid black `M` on a transparent background, shipped as SVG, PNG and ICO.

## Included features

- Native C++20 Windows CLI, WinGet/Chocolatey discovery and delegated installations
- npm project dependencies
- Local copying and optional `.mget` JSON manifest
- Direct HTTPS downloads without persistent package cache
- GitHub source ZIP and interactive release-file selection (public HTML client, no API; tested)
- MGET update-channel support (experimental)
- Turkish and English user documentation

## Known limitations

This is **pre-release software**, not a stable build. GitHub now uses public HTML pages instead of the REST API. Source ZIP download and release asset selection (`2-4-6-8`) were successfully tested using real public GitHub pages. Actual user PATH update during interactive setup and MGET self-update deployment are not fully end-to-end tested. Do not configure untrusted update feeds because signed metadata verification is not yet implemented. `mget remove` only removes directly tracked copied files, not WinGet/Chocolatey packages or registry changes.

Start with [docs/START-HERE.md](START-HERE.md), [Quick Start TR](QUICKSTART-TR.md) or [Quick Start EN](QUICKSTART-EN.md).
