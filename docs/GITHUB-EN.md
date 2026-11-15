# MGET GitHub Integration (EN)

MGET opens public GitHub **HTML pages like a normal web client**. It does not use GitHub's REST API or require an API key.

## Download repository source

```powershell
mget pull https://github.com/OWNER/REPO --loc="C:\Projects"
```

MGET reads the default branch from the public repository page, then downloads its source code ZIP into your chosen directory. The ZIP is not automatically opened or executed.

## Select release assets

```powershell
mget pull https://github.com/OWNER/REPO/releases --loc="C:\Downloads"
```

MGET opens the web page for the latest published release and lists the attached downloadable files with numbers. Example:

```text
[1] DX.exe
[2] program.exe
[3] assets.zip
[4] helper.exe
[5] readme.txt
[6] pack.zip
[7] config.json
[8] portable.exe

Which files? 2-4-6-8
```

**`2-4-6-8`** downloads exactly items **2, 4, 6 and 8**. Any number of distinct choices is supported. `1-3` selects items 1 and 3; **it is not a numeric range**. `0` is invalid, not a special action. Repeated numbers select only one file. Files are downloaded in **the order you typed the numbers**; numbers outside the list are rejected.

Noninteractive examples:

```powershell
mget pull https://github.com/OWNER/REPO/releases --select=2-4-6-8 --loc="C:\Downloads"
mget pull https://github.com/OWNER/REPO/releases --all --loc="C:\Downloads"
mget pull https://github.com/OWNER/REPO/releases/tag/v1.0.0 --select=1-3 --loc="C:\Downloads"
```

`/releases/latest` is supported. For GitHub repository and release URLs, both `mget pull` and `mget install` **download only**; executables are never launched automatically.

## Preview before downloading

```powershell
mget pull https://github.com/OWNER/REPO/releases --select=2-4 --dry-run --loc="C:\Test"
```

`--dry-run` shows selected files without downloading them.

## No cache, known limits

- No persistent download cache; HTML content is processed in memory.
- Files are written directly to `--loc`, or to a folder selected in the Windows picker.
- A temporary `.part` download file can appear in the target folder during a transfer.
- You must confirm before overwriting an existing file.
- Only public repositories and their release assets are supported; private repositories and sign-in are unsupported.
- Changes to GitHub's HTML structure may require an update to MGET's page parser.
- MGET **prints a calculated SHA-256** after downloading; it does not authenticate the publisher or validate a trusted release signature.

[Quick Start](QUICKSTART-EN.md) · [Commands](COMMANDS-EN.md)
