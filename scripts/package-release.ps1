$ErrorActionPreference='Stop'
$root = Split-Path -Parent $PSScriptRoot
$stage=Join-Path $env:TEMP 'mget-release-stage'
$out=Join-Path $root 'release'
if(Test-Path $stage){ Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage -Force | Out-Null
New-Item -ItemType Directory -Path $out -Force | Out-Null
foreach($file in @('README.md','.gitignore')){
  Copy-Item -LiteralPath (Join-Path $root $file) -Destination (Join-Path $stage $file)
}
foreach($folder in @('src','docs','scripts','examples','tests')){
  Copy-Item -LiteralPath (Join-Path $root $folder) -Destination (Join-Path $stage $folder) -Recurse
}
$zip=Join-Path $out 'MGET-v0.1.0-beta.1-source.zip'
if(Test-Path $zip){ Remove-Item -LiteralPath $zip -Force }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal -Force
# Include the dotfile, which PowerShell zip wildcard may omit
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[System.IO.Compression.ZipFile]::Open($zip,[System.IO.Compression.ZipArchiveMode]::Update)
try{
  if(-not $archive.GetEntry('.gitignore')){ [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,(Join-Path $stage '.gitignore'),'.gitignore')|Out-Null }
}finally{$archive.Dispose()}
$setup=Join-Path $root 'build\MGET-Setup.exe'
Copy-Item -LiteralPath $setup -Destination (Join-Path $out 'MGET-Setup.exe') -Force
$checks=@()
foreach($p in @((Join-Path $out 'MGET-Setup.exe'),$zip)){
 $hash=Get-FileHash -LiteralPath $p -Algorithm SHA256
 $checks+=($hash.Hash.ToLower()+'  '+[IO.Path]::GetFileName($p))
}
[IO.File]::WriteAllLines((Join-Path $out 'SHA256SUMS.txt'),$checks)
Get-ChildItem -LiteralPath $out | Select-Object Name,Length
Remove-Item -LiteralPath $stage -Recurse -Force
