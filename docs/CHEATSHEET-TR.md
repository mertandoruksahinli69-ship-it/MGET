# MGET Komut Kopya Kâğıdı

| Ne yapmak istiyorum? | Komut |
|---|---|
| Sürüm | `mget --version` |
| Sağlayıcıları kontrol et | `mget doctor` |
| Uygulama bul | `mget search vscode` |
| WinGet ile yükle | `mget install Microsoft.VisualStudioCode --source=winget` |
| İndir (kurma) | `mget pull https://example.org/dosya.zip --loc="C:\İndirilenler"` |
| GitHub kaynak ZIP | `mget pull https://github.com/OWNER/REPO --loc="C:\Projeler"` |
| GitHub release seç | `mget pull https://github.com/OWNER/REPO/releases --select=1-2 --loc="C:\İndirilenler"` |
| Bütün release dosyalarını seç | `mget pull https://github.com/OWNER/REPO/releases --all --loc="C:\İndirilenler"` |
| Yerel kurulum | `mget install ".\examples\demo" --loc=".\build\test"` |
| Önizleme | `mget install ".\examples\demo" --loc=".\build\test" --dry-run` |
| .mget incele | `mget inspect ".\examples\demo\.mget"` |
| npm bağımlılığı | `mget install npm:lodash --project="."` |
| İzlenen dosyaları gör | `mget list` |
| İzlenen kopyayı kaldır | `mget remove demo.package` |
| MGET'i güncelle | `mget --update` |

**Önemli:** `--select=1-2` sadece **1 ve 2** numaralı dosyaları seçer; `0` özel komut değildir. Dosyalar doğrudan hedefe iner, cache klasörü oluşturulmaz. `OWNER/REPO` ve `example.org` yer tutucudur.

[Hızlı Başlangıç](QUICKSTART-TR.md) · [GitHub Rehberi](GITHUB-TR.md) · [Tam komutlar](COMMANDS-TR.md)
