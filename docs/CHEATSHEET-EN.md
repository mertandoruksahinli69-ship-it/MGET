# MGET Command Cheat Sheet

| Task | Command |
|---|---|
| Version | `mget --version` |
| Diagnostics | `mget doctor` |
| Search | `mget search vscode` |
| Install via WinGet | `mget install Microsoft.VisualStudioCode --source=winget` |
| Download a file | `mget pull https://example.org/file.zip --loc="C:\Downloads"` |
| GitHub source ZIP | `mget pull https://github.com/OWNER/REPO --loc="C:\Projects"` |
| GitHub release asset selection | `mget pull https://github.com/OWNER/REPO/releases --select=1-2 --loc="C:\Downloads"` |
| All release assets | `mget pull https://github.com/OWNER/REPO/releases --all --loc="C:\Downloads"` |
| Install local example | `mget install ".\examples\demo" --loc=".\build\test"` |
| Dry run | `mget install ".\examples\demo" --loc=".\build\test" --dry-run` |
| Inspect manifest | `mget inspect ".\examples\demo\.mget"` |
| npm package | `mget install npm:lodash --project="."` |
| List tracked files | `mget list` |
| Remove tracked copies | `mget remove demo.package` |
| Self-update | `mget --update` |

**Note:** `1-2` selects entries 1 and 2, **not a range**; `0` has no special meaning. Downloads go directly to your selected directory without a persistent cache. `OWNER/REPO` and `example.org` are placeholders.

[Quick Start](QUICKSTART-EN.md) · [GitHub Guide](GITHUB-EN.md) · [Full commands](COMMANDS-EN.md)
