# .mget Manifest Kılavuzu (TR)

**Dosya adı:** `.mget` (uzantısız gizli isim)  
**Format:** UTF-8 JSON  
**Durum:** İsteğe bağlı. Git'teki `.gitignore` gibi proje kökünde durur; ancak kendine özgü JSON sözdizimi vardır.

## Kullanım
```powershell
mget init ".\MyApp"
mget inspect ".\MyApp\.mget"
mget install ".\MyApp" --use-.mget "--loc{C:\Apps\MyApp}" --dry-run
mget install ".\MyApp" --use-.mget "--loc{C:\Apps\MyApp}"
mget install ".\MyApp" --manifest="C:\Tanımlar\özel.mget"
```

## Uygulanmış örnek
```json
{
  "schemaVersion": 1,
  "id": "demo.app",
  "name": "Demo App",
  "version": "1.0.0",
  "install": {
    "mode": "copy",
    "files": [
      { "from": "bin/**", "to": "$INSTALL_DIR/" },
      { "from": "assets/**", "to": "$INSTALL_DIR/assets/" }
    ],
    "registry": [
      {
        "action": "set",
        "hive": "HKCU",
        "key": "Software\\DemoApp",
        "name": "InstallPath",
        "type": "REG_SZ",
        "value": "$INSTALL_DIR"
      }
    ],
    "shortcuts": [
      { "name": "Demo App", "target": "$INSTALL_DIR/Demo.exe", "location": "start_menu" }
    ]
  }
}
```

## Alanlar
| Alan | Anlamı | v0.1 |
|---|---|---|
| `schemaVersion` | Biçim sürümü; `1` | ✅ |
| `id`, `name`, `version` | Kimlik ve sürüm bilgileri | ✅ |
| `install.mode` | `copy` veya `installer` | ✅ |
| `install.files` | `from` → `to` dosya eşlemeleri; `/**` klasör ağacı | ✅ |
| `$INSTALL_DIR` | Kullanıcı tarafından seçilen hedef | ✅ |
| `install.installer` | `installer` modunda göreli EXE/MSI konumu | ✅ |
| `install.registry` | `HKCU/HKLM` içine `REG_SZ` yazma | ✅ |
| `install.shortcuts` | Başlat menüsü kısayolu | ✅ |
| `install.environment.addToPath` | Kurulu paket PATH değişiklikleri | Tasarımda (henüz uygulanmıyor) |
| `dependencies` | Paket bağımlılıkları | Tasarımda |
| `uninstall`, `update`, `hooks` | Geri alma/güncelleme/komut kancaları | Tasarımda |
| `services`, `fileAssociations`, `firewall` | Gelişmiş sistem entegrasyonu | Tasarımda, güvenlik onayı gerektirecek |

## Manifest olmadan
- Yalnız EXE/MSI varsa: kurulum dosyasını **onaydan sonra** açar.
- Yalnız ZIP varsa: hedef klasöre çıkarır.
- Sıradan dosya varsa: hedef klasöre kopyalar.
- Klasör varsa: alt dosyaları hedefe kopyalar.
- `--use-.mget` olup dosya yoksa işlem hata verir.
- Paylaşım URL'si bir web sayfasına yönlenirse, doğrudan dosya indirme olarak işlem görmez.

## Güvenlik kuralları
1. Doğrudan ZIP ve manifestlerde sistem değişikliklerini gözden geçir; onay olmadan HKLM vb. değişiklik yapma.
2. Dosya kopyalama yalnız seçilen hedef dizin içinde; `..` ile dizin dışına çıkış reddedilir.
3. Symlink ile kaynak kaçışı reddedilir.
4. HKLM için Windows izinleri gerekebilir. Uygulama izni kendisi atlamaz.
5. `--dry-run` dosya eşlemelerini gösterir, Windows değişikliklerini uygulamaz.
6. Mevcut v0.1 sürümünde kayıt defteri değişikliklerini tam geri alma desteklenmez.
