$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$exe = Join-Path $env:TEMP 'elevatorlift-core-tests.exe'
$sources = Get-ChildItem -LiteralPath (Join-Path $root 'src/core') -Filter '*.cpp' | ForEach-Object FullName
& g++ -std=c++17 -Wall -Wextra -Werror -static -I (Join-Path $root 'src') (Join-Path $PSScriptRoot 'core_tests.cpp') @sources -o $exe
if ($LASTEXITCODE -ne 0) {throw 'Host compile failed'}
& $exe
if ($LASTEXITCODE -ne 0) {throw 'Host tests failed'}
