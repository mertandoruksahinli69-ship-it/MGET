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
foreach($folder in @('src','docs','scripts','examples','tests','assets')){
  Copy-Item -LiteralPath (Join-Path $root $folder) -Destination (Join-Path $stage $folder) -Recurse
}
$zip=Join-Path $out 'MGET-v0.1.0-beta.2-source.zip'
if(Test-Path $zip){ Remove-Item -LiteralPath $zip -Force }
# Archive every source file once, including dotfiles, with portable forward-slash entry names.
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[System.IO.Compression.ZipFile]::Open($zip,[System.IO.Compression.ZipArchiveMode]::Create)
try{
  Get-ChildItem -LiteralPath $stage -Recurse -File -Force | ForEach-Object {
    $entry=$_.FullName.Substring($stage.Length + 1).Replace('\','/')
    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,$_.FullName,$entry,[System.IO.Compression.CompressionLevel]::Optimal)|Out-Null
  }
}finally{$archive.Dispose()}
$setup=Join-Path $root 'build\MGET-Setup.exe'
Copy-Item -LiteralPath $setup -Destination (Join-Path $out 'MGET-Setup.exe') -Force
Copy-Item -LiteralPath (Join-Path $root 'assets\MGET.png') -Destination (Join-Path $out 'MGET-logo.png') -Force
$checks=@()
foreach($p in @((Join-Path $out 'MGET-Setup.exe'),$zip,(Join-Path $out 'MGET-logo.png'))){
 $hash=Get-FileHash -LiteralPath $p -Algorithm SHA256
 $checks+=($hash.Hash.ToLower()+'  '+[IO.Path]::GetFileName($p))
}
[IO.File]::WriteAllLines((Join-Path $out 'SHA256SUMS.txt'),$checks)
Get-ChildItem -LiteralPath $out | Select-Object Name,Length
Remove-Item -LiteralPath $stage -Recurse -Force
