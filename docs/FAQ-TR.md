# MGET — Sık Sorulan Sorular ve Hatalar

**“mget tanınmıyor”** — Kurulumdan sonra tüm eski terminalleri kapatıp yeni PowerShell/CMD aç. Varsayılan kurulu sürümü doğrudan kontrol etmek için: `& "$env:LOCALAPPDATA\Programs\MGET\mget.exe" --version`.

**“Chocolatey not installed”** — Chocolatey isteğe bağlıdır; `mget doctor` ile bak. WinGet kurulumunda `--source=winget` kullan.

**Kurulum yeri nasıl seçilir?** — Yerel kopyalamalarda `--loc="C:\Apps\Demo"` kullan. Yazmazsan Windows klasör seçimi açılır. Harici EXE/MSI, WinGet ve Chocolatey kurulumlarının klasörünü MGET zorla değiştirmez.

**“.mget required but not found”** — `--use-.mget` zorunluluk getirir. Paketin köküne `.mget` ekle veya bu seçeneği kaldır.

**“Link returned an HTML page”** — Paylaşım sayfası yerine doğrudan HTTPS dosya linki gerekir. GitHub repo ve release bağlantıları özel olarak desteklenir. Google Drive/MediaFire sıradan paylaşım sayfaları henüz desteklenmez.

**GitHub linki kaynak kod mu indirir?** — Depo kök bağlantısında varsayılan dalın kaynak ZIP dosyasını indirir; `/releases` bağlantısında ekleri numaralı listeler. `--select=1-2` sadece 1 ve 2 numaralı dosyaları seçer. `0` özel seçim değildir. [GitHub Kılavuzu](GITHUB-TR.md).

**Cache oluşur mu?** — Paket indirmelerinde kalıcı cache yoktur. Dosyalar hedefe doğrudan indirilir. İndirme sırasında kısa süreli `.part` dosyası oluşabilir; bu bir önbellek değildir.

**`mget pull` ile `mget install` aynı mı?** — Normal doğrudan HTTPS bağlantılarında `pull` yalnız indirir; `install` dosya tipine göre kurulum adımı da yapabilir. GitHub depo/release bağlantılarında ikisi de sadece indirir.

**`mget list` bütün programları gösterir mi?** — Hayır; yalnızca MGET'in kaydettiği yerel dosya kopyalarını listeler.

**`mget remove` kayıt defterini ve WinGet programlarını kaldırır mı?** — Hayır. Yalnızca MGET'in takip ettiği kopyalanmış dosyaları kaldırır. Büyük değişiklikler öncesi yedek önerilir.

**`mget --update` neden çalışmıyor?** — 0.1.x sürümünde resmî yayın akışı yok. Güncelleyiciye güvenilir bir feed tanımlamak gerekir; kriptografik imza doğrulaması henüz uygulanmamıştır.

[Hızlı Başlangıç](QUICKSTART-TR.md) · [Komut Tablosu](CHEATSHEET-TR.md)
