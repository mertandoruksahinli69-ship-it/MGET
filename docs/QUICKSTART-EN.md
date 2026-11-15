# MGET Quick Start

**Get started in 5 minutes.** Version: 0.1.0 developer preview · Windows 10/11.

## 1. Install

1. Open the `build` folder inside your local MGET project directory.
2. Run `MGET-Setup.exe`.
3. In the terminal, answer **Y** to `Install MGET now? [Y/N]` to continue, or **N** to cancel without making changes.
4. Answer **Y** at `Use default installation folder? [Y/N]` to use `%LOCALAPPDATA%\Programs\MGET`, or **N** to type a custom path.
5. Open a **new** PowerShell or CMD window.

```powershell
mget --version
mget doctor
```

Expected output: `MGET 0.1.0`. You can also run `build\mget.exe` directly before installing, but PATH is only configured by setup.

## How do I uninstall MGET?

Run `MGET-Uninstall.exe` from the installed folder, or use **Windows Settings → Installed Apps → MGET**. Answer `Y` to `Uninstall MGET? [Y/N]`. `N` leaves the installation untouched. It removes MGET's installed binaries and matching PATH entry while preserving downloaded packages and your data.

## 2. Search and install

```powershell
mget search vscode
mget install Microsoft.VisualStudioCode --source=winget
```

Use `mget install vscode` to search WinGet and optional Chocolatey and select the provider: **1 = WinGet**, **2 = Chocolatey**, **0 = cancel**. The selected provider performs the actual installation.

## 3. Download a file

```powershell
mget pull https://example.org/archive.zip
mget install https://example.org/setup.exe
```

These are placeholders. Use a real **direct HTTPS file URL from a trusted source**. `pull` only downloads. `install` also handles file installation or copying based on file type; EXE/MSI files request confirmation. Ordinary Google Drive/MediaFire sharing pages are not supported in 0.1.

## 4. Install a local package

Run these commands from the MGET project root:

```powershell
mget install ".\examples\demo" --use-.mget --loc=".\build\demo-test" --dry-run
mget install ".\examples\demo" --use-.mget --loc=".\build\demo-test"
mget list
mget remove demo.package
```

`--dry-run` previews the planned copy; the following command performs it. `remove` only removes tracked copied files, not third-party installers or registry modifications.

## 5. Choose the destination

```powershell
mget install ".\examples\demo" "--loc{C:\Apps\Demo}"
mget install ".\examples\demo" --loc="C:\Apps\Demo"
```

If omitted, `--loc` triggers a Windows folder-picker for local copy, ZIP and manifest workflows. External WinGet/Chocolatey and standalone EXE/MSI installers decide their own destination.

## 6. Optional .mget file

An optional package-root `.mget` **JSON** file describes copy destinations, an installer, selected registry string values and shortcuts.

```powershell
mget inspect ".\examples\demo\.mget"
mget init ".\ExistingPackageFolder"
```

The folder used with `init` must already exist. Use `--use-.mget` to require a manifest; without one, MGET copies files, extracts ZIPs or launches EXE/MSI with confirmation. [Manifest documentation](MANIFEST-EN.md).

## 7. npm support

From an existing Node.js project with npm installed:

```powershell
mget install npm:lodash --project="."
```

MGET delegates dependency installation to npm.

## 8. Updating packages vs MGET itself

```powershell
mget upgrade Microsoft.VisualStudioCode --source=winget
mget --update
```

`upgrade` delegates a package upgrade to WinGet. `--update` is MGET's own updater: a trusted release feed must first be configured. There is no official feed in 0.1, and release signing is not yet implemented.

## 9. Need help?

- **Command not recognized** → open a new terminal.
- **Chocolatey missing** → try `--source=winget`.
- **Manifest missing** → remove `--use-.mget` or create one.
- **HTML returned instead of file** → use a direct HTTPS file link.

[FAQ](FAQ-EN.md) · [Cheat sheet](CHEATSHEET-EN.md)
