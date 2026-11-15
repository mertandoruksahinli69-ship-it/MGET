# MGET: one executable runtime + single-file installer (Windows / LLVM MinGW)
$ErrorActionPreference = "Stop"
$project = Split-Path -Parent $PSScriptRoot
Push-Location $project
try {
    if (!(Get-Command g++ -ErrorAction SilentlyContinue)) { throw "g++ required (LLVM MinGW)." }
    if (!(Get-Command windres -ErrorAction SilentlyContinue)) { throw "windres required (LLVM MinGW)." }
    New-Item -ItemType Directory -Force -Path ".\build" | Out-Null

    Write-Host "[1/3] Building MGET CLI..."
    & g++ -std=c++20 -O2 -Wall -Wextra -municode -static -o ".\build\mget.exe" ".\src\main.cpp" -lwinhttp -lcrypt32 -lole32 -lshell32 -luuid -ladvapi32
    if ($LASTEXITCODE -ne 0) { throw "CLI compilation failed." }

    Write-Host "[2/3] Embedding mget.exe into installer..."
    & windres -i ".\src\installer.rc" -o ".\build\installer-res.o"
    if ($LASTEXITCODE -ne 0) { throw "Resource compilation failed." }

    Write-Host "[3/3] Building single-file installer..."
    & g++ -std=c++20 -O2 -Wall -Wextra -municode -mwindows -static -o ".\build\MGET-Setup.exe" ".\src\installer.cpp" ".\build\installer-res.o" -lole32 -lshell32 -luuid -ladvapi32 -luser32
    if ($LASTEXITCODE -ne 0) { throw "Installer compilation failed." }

    Write-Host "Done: $project\build\MGET-Setup.exe"
    Write-Host "CLI : $project\build\mget.exe"
} finally {
    Pop-Location
}
