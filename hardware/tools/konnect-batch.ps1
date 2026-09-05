param(
    [Parameter(Mandatory = $true)]
    [string]$CallsJson,

    [string[]]$Toolsets = @(),

    [string]$PluginRoot = $env:KONNECT_PLUGIN_ROOT
)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($PluginRoot)) {
    $pluginCandidates = @(
        (Join-Path $env:USERPROFILE 'OneDrive\Documents\KiCad\10.0\3rdparty\plugins\com_github_mixelpixx_konnect'),
        (Join-Path $env:USERPROFILE 'Documents\KiCad\10.0\3rdparty\plugins\com_github_mixelpixx_konnect'),
        (Join-Path $env:APPDATA 'kicad\10.0\3rdparty\plugins\com_github_mixelpixx_konnect')
    )
    $PluginRoot = $pluginCandidates | Where-Object {
        Test-Path -LiteralPath (Join-Path $_ 'bin\konnect.exe')
    } | Select-Object -First 1
}
if ([string]::IsNullOrWhiteSpace($PluginRoot)) {
    throw 'Konnect was not found. Install the KiCad plugin or set KONNECT_PLUGIN_ROOT.'
}
$binary = Join-Path $PluginRoot 'bin\konnect.exe'
$config = Join-Path $PluginRoot 'settings.json'

$startInfo = [Diagnostics.ProcessStartInfo]::new()
$startInfo.FileName = $binary
$startInfo.Arguments = '--config "' + $config + '"'
$startInfo.UseShellExecute = $false
$startInfo.RedirectStandardInput = $true
$startInfo.RedirectStandardOutput = $true

$process = [Diagnostics.Process]::new()
$process.StartInfo = $startInfo
$null = $process.Start()

function Invoke-KonnectMessage {
    param([hashtable]$Message, [int]$Id)

    $process.StandardInput.WriteLine(($Message | ConvertTo-Json -Compress -Depth 40))
    $process.StandardInput.Flush()
    for ($attempt = 0; $attempt -lt 60; $attempt++) {
        $read = $process.StandardOutput.ReadLineAsync()
        if (-not $read.Wait(60000)) {
            throw "Konnect timed out waiting for response id $Id"
        }
        $response = $read.Result | ConvertFrom-Json
        if ($response.id -eq $Id) {
            return $response
        }
    }
    throw "Konnect did not return response id $Id"
}

try {
    $null = Invoke-KonnectMessage -Id 1 -Message @{
        jsonrpc = '2.0'; id = 1; method = 'initialize'
        params = @{
            protocolVersion = '2025-06-18'; capabilities = @{}
            clientInfo = @{ name = 'Codex'; version = '1' }
        }
    }
    $process.StandardInput.WriteLine('{"jsonrpc":"2.0","method":"notifications/initialized"}')
    $process.StandardInput.Flush()

    $requestId = 2
    if ($Toolsets.Count -gt 0) {
        $null = Invoke-KonnectMessage -Id $requestId -Message @{
            jsonrpc = '2.0'; id = $requestId; method = 'tools/call'
            params = @{ name = 'load_toolset'; arguments = @{ name = $Toolsets } }
        }
        $requestId++
    }

    $calls = @($CallsJson | ConvertFrom-Json -AsHashtable)
    $results = @()
    foreach ($call in $calls) {
        $response = Invoke-KonnectMessage -Id $requestId -Message @{
            jsonrpc = '2.0'; id = $requestId; method = 'tools/call'
            params = @{ name = $call.tool; arguments = $call.arguments }
        }
        $entry = @{ tool = $call.tool; isError = [bool]$response.result.isError }
        if ($response.result.content.Count -gt 0) {
            $entry.result = $response.result.content[0].text
        }
        $results += $entry
        $requestId++
    }

    $results | ConvertTo-Json -Depth 40
}
finally {
    if (-not $process.HasExited) {
        $process.Kill()
    }
}
