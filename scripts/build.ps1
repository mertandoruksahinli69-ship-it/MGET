# Build MGET CLI and console installer, embedding both app and uninstaller.
$ErrorActionPreference = "Stop"
$project = Split-Path -Parent $PSScriptRoot
Push-Location $project
try {
  foreach($name in @("g++","windres","python")){
    if (!(Get-Command $name -ErrorAction SilentlyContinue)) { throw "$name is required." }
  }
  New-Item -ItemType Directory -Force -Path ".\build",".\assets" | Out-Null
  Write-Host "[1/5] Generating MGET black-M icon"
  & python ".\scripts\generate-icon.py"
  if ($LASTEXITCODE -ne 0) { throw "Icon generation failed (install Pillow)." }

  Write-Host "[2/5] Compiling standalone CLI"
  & g++ -std=c++20 -O2 -Wall -Wextra -municode -mconsole -static -o ".\build\mget.exe" ".\src\main.cpp" -lwinhttp -lcrypt32 -lole32 -lshell32 -luuid -ladvapi32
  if ($LASTEXITCODE -ne 0) { throw "CLI build failed." }

  Write-Host "[3/5] Embedding M icon into the uninstaller"
  & windres -i ".\src\uninstaller.rc" -o ".\build\uninstaller-res.o"
  if ($LASTEXITCODE -ne 0) { throw "Uninstaller icon resource failed." }

  Write-Host "[4/5] Compiling terminal uninstaller"
  & g++ -std=c++20 -O2 -Wall -Wextra -municode -mconsole -static -o ".\build\MGET-Uninstall.exe" ".\src\uninstaller.cpp" ".\build\uninstaller-res.o" -ladvapi32 -luser32
  if ($LASTEXITCODE -ne 0) { throw "Uninstaller build failed." }

  Write-Host "[5/5] Embedding CLI + uninstaller into one setup EXE"
  & windres -i ".\src\installer.rc" -o ".\build\installer-res.o"
  if ($LASTEXITCODE -ne 0) { throw "Installer resources failed." }
  & g++ -std=c++20 -O2 -Wall -Wextra -municode -mconsole -static -o ".\build\MGET-Setup.exe" ".\src\installer.cpp" ".\build\installer-res.o" -lole32 -lshell32 -luuid -ladvapi32 -luser32
  if ($LASTEXITCODE -ne 0) { throw "Installer build failed." }

  Write-Host "Built: .\build\MGET-Setup.exe"
  Write-Host "Embedded: mget.exe + MGET-Uninstall.exe + black M icon"
} finally { Pop-Location }
