# MGET — Komutlar (TR)

Sürüm: **0.1.0 geliştirici önizlemesi**.

## Genel kullanım

| Komut | Açıklama |
|---|---|
| `mget help` | Yardım |
| `mget --version` | Sürümü göster |
| `mget doctor` | Sağlayıcıları, PATH ile ilgili araçları, kanal ayarını denetle |
| `mget search <isim>` | WinGet ve Chocolatey üzerinde arama |
| `mget search <isim> --source=winget` | Yalnızca WinGet |
| `mget search <isim> --source=choco` | Yalnızca Chocolatey |
| `mget search <isim> --source=npm` | NPM'de arama |
| `mget install <isim>` | Kaynak araması ve interaktif sağlayıcı seçimi |
| `mget install <isim> --source=winget` | WinGet'e devredilen kurulum |
| `mget install <isim> --source=choco` | Chocolatey'e devredilen kurulum |
| `mget install npm:<isim> --project=<dizin>` | Proje klasöründe `npm install` |
| `mget install <HTTPS URL>` | Dosyayı indirir, dosya tipine göre kurar/kopyalar |
| `mget install <yerel dosya veya klasör>` | Yerel paket kurulumu |
| `mget install <paket> --use-.mget` | Manifest zorunlu; yoksa hata |
| `mget install <paket> --manifest=<dosya>` | Belirli manifesti kullan |
| `mget inspect <dosya.mget>` | Manifest bilgilerini göster |
| `mget init [proje klasörü]` | Örnek `.mget` üret |
| `mget pull <HTTPS URL>` | Kurmadan doğrudan hedefe indir; SHA-256 göster |
| `mget pull https://github.com/OWNER/REPO` | GitHub varsayılan dal kaynak ZIP dosyasını indir |
| `mget pull https://github.com/OWNER/REPO/releases --select=1-2` | Release dosyalarından 1 ve 2'yi indir |
| `mget pull https://github.com/OWNER/REPO/releases --all` | Release içindeki bütün dosyaları indir |
| `mget list` | İzlenen yerel dosyaları listele |
| `mget remove <paket>` | MGET'in izlediği kopyalanmış dosyaları onayla kaldır |
| `mget upgrade [paket]` | WinGet'e yükseltme çağrısı |
| `mget upgrade <paket> --source=choco` | Chocolatey yükseltmesi |
| `mget deploy <yerel dizin> --target=<dizin>` | Yerel dağıtım/kopyalama |
| `mget channel <stable\|beta\|nightly>` | Kalıcı güncelleme kanalı |
| `mget channel stable --feed=https://site/releases/` | HTTPS güncelleme kaynağı tanımla |
| `mget --update` | MGET'in kendisini güncelle |
| `mget --update --check` | Yeni sürümü kontrol/indir/doğrula; uygulama |
| `mget --update --channel=beta` | Bir seferlik kanal seç |

## Konum seçenekleri

```powershell
mget install ".\Demo" "--loc{C:\Uygulamalar\Demo}"
mget install ".\Demo" --loc="C:\Uygulamalar\Demo"
mget install ".\Demo" "--loc{kullanıcıadı/İndirilenler}"
mget install ".\Demo" "--loc{%USERPROFILE%\Downloads}"
```

`--loc` olmadan yerel kurulumda Windows klasör seçim penceresi açılır. `winget` veya `choco` kendi kurulum aracını yönetir; `--loc` harici kurucunun hedefini zorla değiştiremez. EXE/MSI dosyaları çalıştırılmadan önce onay istenir. Sıradan tek dosya, hedefe kopyalanır. ZIP, hedefe çıkarılır.

## .mget davranışı
- Proje kökünde **isteğe bağlı** `.mget` JSON manifesti yer alır.
- Dosya bulunduğunda manifest geçerlidir; `--use-.mget` dosyayı zorunlu kılar.
- Dosya olmadan tek kurulum EXE/MSI açılır, ZIP açılır, diğer dosyalar kopyalanır.
- Registry/Start Menu işlemlerinde onay istenir. `--dry-run` temel dosya işlemlerinin önizlemesini verir.
- Manifest güncel v0.1 için yalnızca `copy` ve `installer` modlarını destekler.

## Güncelleme kanalı
`mget --update` **MGET uygulamasını** günceller. Önce `mget channel stable --feed=https://sunucu.example/releases/` ile kendi sürüm akışının taban URL'sini tanımla.

Kanal dosyası: `stable.json`, `beta.json` veya `nightly.json`.

Örnek JSON:
```json
{"version":"0.1.1","url":"https://sunucu.example/releases/mget-0.1.1.exe","sha256":"<64 karakter SHA-256 hex>"}
```
SHA-256 manifestten okunur; bu prototipte imza tabanlı kaynak doğrulama **yoktur**. Henüz gerçek bir resmî kanal yayınlanmadı.

## Kurulum EXE
`build\MGET-Setup.exe` tek kurulum dosyasıdır; içinde `mget.exe` bulunur. Varsayılan hedef `%LOCALAPPDATA%\Programs\MGET` (yönetici izni gerekmez). Kullanıcı yeni dizin de seçebilir. Setup, `HKCU\Environment\Path` değerine MGET klasörünü bir kez ekler ve Windows'a ortam değişikliğini bildirir. Yeni terminal açınca `mget` doğrudan çalışır.

**Sınırlamalar:** Kayıt defteri değişiklikleri ve harici WinGet/Chocolatey kurulumları `mget remove` ile geri alınmaz. NPM, Node.js gerektirir. Google Drive/MediaFire paylaşım sayfalarından dosya bağlantısı ayrıştırma planlanmıştır; v0.1'de yalnızca doğrudan HTTPS dosya URL'leri işlenir.
