[CmdletBinding()]
param(
    [ValidateSet('Version', 'Validate', 'Gui')]
    [string]$Action = 'Validate',

    [string]$ReportDirectory,

    [string]$KiCadBin = $env:KICAD_10_BIN
)

$ErrorActionPreference = 'Stop'

# Keep automated work pinned to the KiCad major version used by this project,
# while allowing a non-default installation on another computer.
if ([string]::IsNullOrWhiteSpace($KiCadBin)) {
    $KiCadBin = Join-Path $env:ProgramFiles 'KiCad\10.0\bin'
}
$KiCadCli = Join-Path $KiCadBin 'kicad-cli.exe'
$KiCadGui = Join-Path $KiCadBin 'kicad.exe'
$HardwareDirectory = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Schematic = Join-Path $HardwareDirectory 'ElevatorLift.kicad_sch'
$Board = Join-Path $HardwareDirectory 'ElevatorLift.kicad_pcb'
$Project = Join-Path $HardwareDirectory 'ElevatorLift.kicad_pro'

if ([string]::IsNullOrWhiteSpace($ReportDirectory)) {
    $ReportDirectory = Join-Path $HardwareDirectory 'reports'
}

if (-not (Test-Path -LiteralPath $KiCadCli)) {
    throw "KiCad 10 command-line tool was not found at: $KiCadCli. Set KICAD_10_BIN or pass -KiCadBin for a non-default installation."
}

switch ($Action) {
    'Version' {
        & $KiCadCli --version
        break
    }

    'Validate' {
        $resolvedReportDirectory = [System.IO.Path]::GetFullPath($ReportDirectory)
        New-Item -ItemType Directory -Force -Path $resolvedReportDirectory | Out-Null

        & $KiCadCli sch erc $Schematic --output (Join-Path $resolvedReportDirectory 'erc.json') --format json --severity-all
        if ($LASTEXITCODE -ne 0) { throw "KiCad ERC failed with exit code $LASTEXITCODE." }

        & $KiCadCli pcb drc $Board --output (Join-Path $resolvedReportDirectory 'drc.json') --format json --severity-all
        if ($LASTEXITCODE -ne 0) { throw "KiCad DRC failed with exit code $LASTEXITCODE." }

        Write-Output "Reports written to $resolvedReportDirectory"
        break
    }

    'Gui' {
        if (-not (Test-Path -LiteralPath $KiCadGui)) {
            throw "KiCad 10 GUI was not found at: $KiCadGui"
        }
        Start-Process -FilePath $KiCadGui -ArgumentList $Project
        break
    }
}
