$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$out = Join-Path $env:TEMP 'elevatorlift-integration-review-20260910.exe'
$sources = Get-ChildItem -LiteralPath (Join-Path $root 'firmware/src') -Filter '*.cpp' | Where-Object Name -ne 'main.cpp' | ForEach-Object FullName
& 'C:/msys64/ucrt64/bin/g++.exe' -std=c++17 -O0 -static '-I' (Join-Path $PSScriptRoot 'host') '-I' (Join-Path $root 'firmware/include') '-I' (Join-Path $root 'firmware/src') (Join-Path $PSScriptRoot 'host/review.cpp') @sources -o $out
if ($LASTEXITCODE -ne 0) {throw 'Host compilation failed'}
& $out | Tee-Object -FilePath (Join-Path $PSScriptRoot 'host-results.txt')
if ($LASTEXITCODE -ne 0) {throw 'A review reproduction did not match its expected observation'}
