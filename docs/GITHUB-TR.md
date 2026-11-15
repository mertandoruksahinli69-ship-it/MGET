# MGET GitHub Kullanımı (TR)

MGET, herkese açık GitHub sayfalarını HTTPS üzerinden **normal bir istemci gibi açar**. GitHub REST API'sini veya API anahtarını kullanmaz.

## Kaynak kod indir

```powershell
mget pull https://github.com/OWNER/REPO --loc="C:\Projeler"
```

Depo sayfasından varsayılan dal belirlenir. Dalın kaynak kodu ZIP olarak seçilen klasöre kaydedilir. Kendiliğinden çalıştırılmaz veya açılmaz.

## Releases içindeki dosyalardan seç

```powershell
mget pull https://github.com/OWNER/REPO/releases --loc="C:\İndirilenler"
```

MGET son yayınlanan release'in web sayfasını açıp indirilebilir ekleri numaralı listeler. Örnek:

```text
[1] DX.exe
[2] program.exe
[3] assets.zip
[4] helper.exe
[5] readme.txt
[6] pack.zip
[7] config.json
[8] portable.exe

Hangileri insin?
Seçim: 2-4-6-8
```

**`2-4-6-8`** yalnız **2, 4, 6 ve 8** numaralı dosyaları indirir. İstediğin kadar numara yazabilirsin. `1-3`, 1 ve 3'ü seçer; **bir aralık değildir**. `0` özel komut değildir ve geçersiz sayılır. Tekrarlanan numaralar yalnız bir kez indirilir. **İndirme sırası**, numaraları yazdığın sıradır; listede bulunmayan numara kabul edilmez.

Seçimi komutla belirlemek için:

```powershell
mget pull https://github.com/OWNER/REPO/releases --select=2-4-6-8 --loc="C:\İndirilenler"
mget pull https://github.com/OWNER/REPO/releases --all --loc="C:\İndirilenler"
mget pull https://github.com/OWNER/REPO/releases/tag/v1.0.0 --select=1-3 --loc="C:\İndirilenler"
```

`/releases/latest` bağlantısı da desteklenir. `mget install` ve `mget pull` GitHub depo veya release bağlantısında **yalnız indirir**; EXE otomatik çalıştırılmaz.

## Önizleme

```powershell
mget pull https://github.com/OWNER/REPO/releases --select=2-4 --dry-run --loc="C:\Test"
```

`--dry-run` indirme yapmadan seçilen dosyaları gösterir.

## Önbellek ve sınırlamalar

- **Kalıcı cache oluşturulmaz.** HTML sayfaları RAM'de işlenir.
- Dosyalar doğrudan `--loc` klasörüne, belirtilmemişse Windows klasör seçme penceresinde seçilen konuma kaydedilir.
- İndirme sırasında hedefte `.part` adlı geçici bir dosya oluşabilir; işlem bitince asıl adına dönüşür.
- Var olan dosyanın üzerine yazmak için onay gerekir.
- Sadece herkese açık GitHub depoları ve release dosyaları desteklenir. Oturum açma, private repo ve giriş gerektiren indirmeler yoktur.
- GitHub'ın HTML sayfa düzeni değişirse dosya listeleme güncelleme isteyebilir.
- MGET indirilen dosyanın **hesaplanan SHA-256 değerini gösterir**; bunu yayıncıya ait güvenilir bir imzayla doğrulamaz.

[Hızlı Başlangıç](QUICKSTART-TR.md) · [Komutlar](COMMANDS-TR.md)
