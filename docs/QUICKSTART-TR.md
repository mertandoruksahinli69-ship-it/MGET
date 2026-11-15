# MGET Hızlı Başlangıç

**5 dakikada MGET kullanmayı öğren.** Sürüm: 0.1.0 · Platform: Windows 10/11.

## 1. Programı kur

1. MGET proje klasöründeki `build` klasörüne gir.
2. `MGET-Setup.exe` dosyasını aç.
3. Açılan terminalde `Install MGET now? [Y/N]` sorusuna `Y` yaz. `N` hiçbir şey yüklemez.
4. `Use default installation folder? [Y/N]` sorusuna `Y` yazarak `%LOCALAPPDATA%\Programs\MGET` dizinini seç; `N` ile başka klasör yolu yaz.
5. İşlem tamamlanınca **yeni** PowerShell veya CMD aç.

```powershell
mget --version
mget doctor
```

`mget --version` çıktısı `MGET 0.1.0` olmalı. Kurulum yapmadan test etmek için `build\mget.exe` doğrudan çalıştırılabilir.

## MGET'i nasıl kaldırırım?

Kurulum klasöründeki `MGET-Uninstall.exe` dosyasını aç veya Windows **Yüklü Uygulamalar → MGET** bölümünü kullan. Terminalde `Uninstall MGET? [Y/N]` sorusuna `Y` yaz. `N` hiçbir şeyi kaldırmaz. Kaldırıcı programın kendisini ve PATH kaydını siler; **indirilen paketlerine dokunmaz**.

## 2. Uygulama bul ve kur

```powershell
mget search vscode
mget install Microsoft.VisualStudioCode --source=winget
```

Sağlayıcıyı kendin seçmek için:

```powershell
mget install vscode
```

MGET, WinGet ve kuruluysa Chocolatey üzerinde arar. Ardından **1 = WinGet**, **2 = Chocolatey**, **0 = İptal** seçenekleri gelir. Gerçek kurulum seçtiğin sağlayıcı üzerinden gerçekleşir.

## 3. Dosya indir

**Sadece indir:**

```powershell
mget pull https://example.org/program.zip
```

**İndir ve dosya türüne göre kur/kopyala:**

```powershell
mget install https://example.org/kurulum.exe
```

`example.org` adresi yalnızca **örnek**tir; yerine güvendiğin kaynağın **doğrudan HTTPS dosya bağlantısını** koy. EXE/MSI çalıştırılmadan önce onay istenir. Google Drive ve MediaFire'ın normal paylaşım sayfaları şu an otomatik dosya linkine dönüştürülmez.

## 4. Yerel dosya ve klasör kur

MGET proje klasöründe bulunan `examples\demo` örneğiyle deneyebilirsin.

**Önizleme (hiçbir dosya kopyalanmaz):**

```powershell
mget install ".\examples\demo" --use-.mget --loc=".\build\deneme" --dry-run
```

**Gerçek kurulum:**

```powershell
mget install ".\examples\demo" --use-.mget --loc=".\build\deneme"
mget list
```

**Örnek paketi kaldır:**

```powershell
mget remove demo.package
```

Silmeden önce onay istenir. Kaldırma işlemi sadece MGET tarafından kaydedilen kopyalanmış dosyalar içindir; başka programların kurulumunu veya Registry değişikliklerini geri almaz.

## 5. Kurulum yerini belirle

```powershell
mget install ".\examples\demo" "--loc{C:\Apps\Demo}"
mget install ".\examples\demo" --loc="C:\Apps\Demo"
```

**`--loc` yazmazsan** yerel klasör, ZIP veya manifest kurulumunda Windows **klasör seçme penceresi** açılır. Tek başına EXE/MSI kurucuları ile WinGet/Chocolatey'nin hedef klasörü, harici kurucu tarafından yönetilir. `--loc` bunu zorla değiştirmez.

## 6. .mget dosyası nedir?

Paket kökünde bulunabilen, **zorunlu olmayan JSON ayar dosyasıdır.** Dosyaların nereye kopyalanacağını, kurulum dosyasını, bazı kayıt defteri girdilerini ve kısayolları belirleyebilir.

Mevcut örneği incele:

```powershell
mget inspect ".\examples\demo\.mget"
```

Mevcut bir klasörde yeni manifest oluştur:

```powershell
mget init ".\MevcutPaketKlasoru"
```

`--use-.mget` yazarsan manifest zorunlu olur. Yazmazsan manifest varsa kullanılır; yoksa EXE/MSI açılır, ZIP çıkarılır veya dosyalar kopyalanır. Bütün ayrıntılar: [.mget rehberi](MANIFEST-TR.md).

## 7. NPM paketi kur

Node.js/NPM kuruluysa, **mevcut** bir Node.js proje klasörünün içindeyken:

```powershell
mget install npm:lodash --project="."
```

Bu komut NPM'i çağırır; MGET NPM'in bağımlılık yönetimi motorunun yerini almaz.

## 8. Güncelleme farkı

```powershell
mget upgrade Microsoft.VisualStudioCode --source=winget
mget --update
```

**`upgrade`** seçilen paketi, **`--update`** MGET'in kendisini günceller. 0.1.0 için henüz resmî güncelleme kaynağı yoktur. Güncelleyici için güvenilir HTTPS yayın akışı gerekir ve imzalı yayın doğrulaması henüz bulunmaz.

## 9. Sorun mu var?

- **"mget tanınmıyor"** → Yeni terminal aç; kurulumun tamamlandığını kontrol et.
- **"Chocolatey not installed"** → `--source=winget` seç.
- **".mget required but not found"** → `--use-.mget` kaldır ya da dosyayı ekle.
- **"HTML page"** → İndirme sayfası değil, doğrudan HTTPS dosya linki kullan.

[SSS ve Hata Çözümleri](FAQ-TR.md) · [Kısa Komut Listesi](CHEATSHEET-TR.md)
