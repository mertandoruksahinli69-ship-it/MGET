# .mget Manifest Reference (EN)

**Filename:** `.mget` | **Encoding:** UTF-8 JSON | **Required:** No.  
Similar to how `.gitignore` lives at a repository root, `.mget` can live in a package root, but its syntax is **JSON**.

## Quick start
```powershell
mget init ".\Example"
mget inspect ".\Example\.mget"
mget install ".\Example" --use-.mget "--loc{C:\Apps\Example}" --dry-run
mget install ".\Example" "--loc{C:\Apps\Example}"
mget install ".\Example" --manifest="C:\Manifests\custom.mget"
```

## Working schema v1 example
```json
{
  "schemaVersion": 1,
  "id": "example.app",
  "name": "Example App",
  "version": "1.0.0",
  "install": {
    "mode": "copy",
    "files": [
      {"from": "bin/**", "to": "$INSTALL_DIR/"},
      {"from": "assets/**", "to": "$INSTALL_DIR/assets/"}
    ],
    "registry": [
      {"action": "set", "hive": "HKCU", "key": "Software\\ExampleApp",
       "name": "InstallPath", "type": "REG_SZ", "value": "$INSTALL_DIR"}
    ],
    "shortcuts": [
      {"name": "Example", "target": "$INSTALL_DIR/Example.exe", "location": "start_menu"}
    ]
  }
}
```

## Field support
| Field | Meaning | v0.1 |
|---|---|---|
| `schemaVersion` | Schema version 1 | Implemented |
| `id`, `name`, `version` | Identity and metadata | Implemented |
| `install.mode` | `copy` or `installer` | Implemented |
| `install.files` | Array of `from` / `to` maps; `/**` copies directories | Implemented |
| `$INSTALL_DIR` | Selected installation directory | Implemented |
| `install.installer` | Relative EXE or MSI in installer mode | Implemented |
| `install.registry` | `HKCU` / `HKLM` string registry values | Implemented |
| `install.shortcuts` | Start Menu links | Implemented |
| `install.environment.addToPath` | Package PATH additions | Planned; currently only displays a notice |
| `dependencies` | Dependency solver | Planned |
| `uninstall`, `update`, `hooks` | Reversible actions, updates and hooks | Planned |
| `services`, `fileAssociations`, `firewall` | Extended system integration | Planned, consent required |

## If no .mget file exists
- A single EXE/MSI installer is launched after confirmation.
- A ZIP archive is extracted to the chosen folder.
- Other single files are copied to the chosen folder.
- A directory is recursively copied into the chosen folder.
- `--use-.mget` makes the manifest mandatory.
- Only direct HTTPS download links are supported; shared web landing pages are not parsed.

## Safety
Installation paths cannot escape the chosen root and source symlinks are rejected. System modifications are confirmed interactively. Registry rollback, comprehensive dependency resolution and signed manifest verification are not part of v0.1.
