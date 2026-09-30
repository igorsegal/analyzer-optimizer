# dump_repo.ps1 - ASCII only
# Usage: powershell -ExecutionPolicy Bypass -File dump_repo.ps1

$ErrorActionPreference = 'Stop'

$root = $PSScriptRoot
if (-not $root) { $root = (Get-Location).Path }
$out  = Join-Path $root '_dump.txt'

$excludeDirs = @('.git','build','out','bin','obj','.vs','x64','Release','Debug','RelWithDebInfo','MinSizeRel','CMakeFiles','ipch')

$includeExt = @('.h','.hpp','.cpp','.c','.cc','.cxx','.txt','.md','.ini','.cmake','.ps1','.py','.json','.yaml','.yml','.csv','.bat','.cmd')

$includeNames = @('CMakeLists.txt','Makefile','.gitignore','.gitattributes')

$maxSize = 5MB

$files = Get-ChildItem -Path $root -Recurse -File | Where-Object {
    $rel = $_.FullName.Substring($root.Length).TrimStart('\')
    $parts = $rel -split '\\'
    $skip = $false
    if ($parts.Length -gt 1) {
        $dirs = $parts[0..($parts.Length-2)]
        foreach ($p in $dirs) {
            if ($excludeDirs -contains $p) { $skip = $true; break }
        }
    }
    if ($skip) { return $false }
    if ($_.Length -gt $maxSize) { return $false }
    if ($includeNames -contains $_.Name) { return $true }
    if ($includeExt -contains $_.Extension.ToLower()) { return $true }
    return $false
} | Sort-Object FullName

"" | Set-Content -Path $out -Encoding UTF8
"# DUMP OF: $root" | Add-Content -Path $out -Encoding UTF8
"# Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" | Add-Content -Path $out -Encoding UTF8
"# Files: $($files.Count)" | Add-Content -Path $out -Encoding UTF8
"" | Add-Content -Path $out -Encoding UTF8

foreach ($f in $files) {
    $rel = $f.FullName.Substring($root.Length).TrimStart('\')
    "===== FILE: $rel =====" | Add-Content -Path $out -Encoding UTF8
    "----- SIZE: $($f.Length) bytes -----" | Add-Content -Path $out -Encoding UTF8
    try {
        $content = Get-Content -LiteralPath $f.FullName -Raw -Encoding UTF8
        $content | Add-Content -Path $out -Encoding UTF8
    } catch {
        "[ERROR reading file: $_]" | Add-Content -Path $out -Encoding UTF8
    }
    "" | Add-Content -Path $out -Encoding UTF8
    "===== END: $rel =====" | Add-Content -Path $out -Encoding UTF8
    "" | Add-Content -Path $out -Encoding UTF8
}

Write-Host "Done: $out"
Write-Host "Files: $($files.Count)"
Write-Host "Size KB: $([Math]::Round((Get-Item $out).Length/1KB,1))"