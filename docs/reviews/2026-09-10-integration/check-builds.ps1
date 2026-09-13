$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$checks = @(
  @{name='firmware';dir='firmware';command='pio';args=@('run')},
  @{name='web TypeScript';dir='webapp';command='npm.cmd';args=@('run','check')},
  @{name='web embedded build';dir='webapp';command='npm.cmd';args=@('run','build:embedded')},
  @{name='web SSR smoke test';dir='webapp';command='npm.cmd';args=@('test')},
  @{name='web lint';dir='webapp';command='npm.cmd';args=@('run','lint')}
)
$results = @()
foreach ($check in $checks) {
  Push-Location (Join-Path $root $check.dir)
  try {
    $output = & $check.command @($check.args) 2>&1 | Out-String
    $code = $LASTEXITCODE
    $results += @{name=$check.name;command=($check.command+' '+($check.args -join ' '));exitCode=$code;output=$output}
    Write-Output "$($check.name): exit $code"
  } finally { Pop-Location }
}
$results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'build-results.json') -Encoding utf8
if (@($results | Where-Object exitCode -ne 0).Count -gt 0) {throw 'One or more build checks failed; inspect build-results.json'}
