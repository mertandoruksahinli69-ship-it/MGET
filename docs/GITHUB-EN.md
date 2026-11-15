# MGET GitHub Integration (EN)

**Version 0.1.x:** Download a repository's source ZIP or interactively select release assets.

## Repository URL → source ZIP

```powershell
mget pull https://github.com/OWNER/REPO --loc="C:\Projects"
```

MGET reads the repository's default branch from the public GitHub API and downloads its **source code ZIP** directly to your destination. The ZIP is not executed or extracted automatically.

## Release URL → numbered asset list

```powershell
mget install https://github.com/OWNER/REPO/releases --loc="C:\Downloads"
```

Illustrative output:

```text
GitHub release: OWNER/REPO (v1.0)
 1  DX.exe
 2  program.exe
 3  assets.zip
Which files? Example: 1-2-4
Selection:
```

Type `1` for DX.exe, `1-2` for DX.exe **and** program.exe, or `3-1-2` for all three. Hyphens **separate individual choices; they do not indicate numeric ranges**. `0` (or `0-0-0`) is **not** a special command and will be rejected. Use `--all` explicitly to download all release assets.

Noninteractive examples:

```powershell
mget pull https://github.com/OWNER/REPO/releases --select=1-2 --loc="C:\Downloads"
mget pull https://github.com/OWNER/REPO/releases --all --loc="C:\Downloads"
mget pull https://github.com/OWNER/REPO/releases/tag/v1.0.0 --select=1 --loc="C:\Downloads"
```

`/releases` and `/releases/latest` use GitHub's latest stable published release. Releases without attached files are reported without downloading anything.

**Important:** MGET **downloads only** for GitHub repository/release URLs; it does not auto-execute EXE assets. GitHub `install` and `pull` therefore share this behavior.

## No cache

- GitHub API JSON is handled **in memory**; no persistent package cache is created.
- Artifacts are written directly to `--loc` (or a folder chosen using the Windows picker).
- A temporary `.part` download file can exist **inside the target directory** during a transfer and is renamed after completion.
- MGET asks before overwriting existing files.
- Only public GitHub resources are supported; GitHub authentication and private repositories are not implemented.

**Limits:** GitHub API may rate-limit unauthenticated requests. Verify untrusted executable downloads before running. An asset's SHA-256 is checked when GitHub supplies a corresponding digest field.

[Quick Start](QUICKSTART-EN.md) · [Full Commands](COMMANDS-EN.md)
