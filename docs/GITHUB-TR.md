# MGET GitHub Kullanımı (TR)

**Sürüm 0.1.x:** GitHub bağlantıları için kaynak ZIP ve numaralı release dosyası seçimi.

## Depo bağlantısı → kaynak kodu ZIP

```powershell
mget pull https://github.com/OWNER/REPO --loc="C:\Projeler"
```

`releases` olmayan normal depo bağlantısında GitHub API üzerinden **varsayılan dal** bulunur; kaynak kod doğrudan hedef klasöre ZIP olarak indirilir. ZIP kendiliğinden çalıştırılmaz veya açılmaz.

## Releases bağlantısı → numaralı dosya listesi

```powershell
mget install https://github.com/OWNER/REPO/releases --loc="C:\İndirilenler"
```

Örnek ekran (dosya adları temsilidir):

```text
GitHub release: OWNER/REPO (v1.0)
 1  DX.exe
 2  program.exe
 3  assets.zip

Which files? Example: 1-2-4
Selection:
```

`1` yazarsan yalnız **DX.exe**, `1-2` yazarsan **DX.exe ve program.exe**, `3-1-2` yazarsan **üçü** seçilir. Tire işareti **aralık değil, çoklu seçim ayırıcıdır**. `0` veya `0-0-0` özel bir komut değildir; geçersiz seçim hatası verir. **Her dosyayı** seçmek istersen açıkça `--all` kullan.

Seçimi tek komutta belirt:

```powershell
mget pull https://github.com/OWNER/REPO/releases --select=1-2 --loc="C:\İndirilenler"
mget pull https://github.com/OWNER/REPO/releases --all --loc="C:\İndirilenler"
```

Belirli sürüm:

```powershell
mget pull https://github.com/OWNER/REPO/releases/tag/v1.0.0 --select=1 --loc="C:\İndirilenler"
```

`/releases` ve `/releases/latest` bağlantıları **en son kararlı yayımlanmış release** öğesini kullanır. Varsa yayın ekleri listelenir; release'te ek yoksa bilgi verir.

**Not:** GitHub release dosyaları bu aşamada yalnızca indirilir; EXE'ler otomatik başlatılmaz. Bu nedenle `mget pull` ve `mget install` GitHub depo/release bağlantılarında aynı indirme seçimini sunar.

## Cache yok

- **Kalıcı cache klasörü oluşturulmaz.** GitHub API yanıtları RAM'de okunur.
- Her dosya, `--loc` hedef klasörüne **doğrudan** indirilir; `--loc` yoksa Windows klasör seçim penceresi açılır.
- İndirme sürerken hedef klasörde geçici `.part` dosyası bulunabilir; başarılı indirme sonunda asıl dosya adına dönüştürülür.
- Kaynak kod veya release dosyaları `%LOCALAPPDATA%\MGET\cache` içine kaydedilmez.
- Mevcut dosya üzerine yazma öncesi onay istenir.
- API kullanımı herkese açık depolar içindir; özel depolara giriş ve GitHub token desteği bu sürümde yoktur.

## Sınırlar

GitHub API'nin hız sınırları, ağ hataları, büyük boyut sınırları ve release sahibi tarafından paylaşılmış dosyaların güvenilirliği önemlidir. Tanımadığın exe'yi çalıştırmadan önce kontrol et. Release varlıklarının GitHub API'de verilen SHA-256 özetleri bulunuyorsa doğrulanır; her projede bu alan sağlanmayabilir.

[Hızlı Başlangıç](QUICKSTART-TR.md) · [Komutlar](COMMANDS-TR.md)
