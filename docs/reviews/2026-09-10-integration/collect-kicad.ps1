param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$hw = Join-Path $root 'hardware'
$sheets = @('ElevatorLift','Power_RevA','MCU_Storage_RevA','VFD_Interface_RevA','Encoder_Counter_RevA','IO')
$calls = @()
foreach ($sheet in $sheets) {
  foreach ($tool in @('list_schematic_components','find_shorted_nets','find_orphan_items','find_single_pin_nets')) {
    $calls += @{tool=$tool; arguments=@{schematic=(Join-Path $hw "$sheet.kicad_sch")}}
  }
}
$calls += @{tool='export_netlist'; arguments=@{board=(Join-Path $hw 'ElevatorLift.kicad_sch');output=(Join-Path $PSScriptRoot 'current.net');format='kicad'}}
$calls += @{tool='run_erc'; arguments=@{schematic=(Join-Path $hw 'ElevatorLift.kicad_sch');severity='warning';output=(Join-Path $PSScriptRoot 'erc.json')}}
$calls += @{tool='get_drc_violations'; arguments=@{board=(Join-Path $hw 'ElevatorLift.kicad_pcb');severity='warning';output=(Join-Path $PSScriptRoot 'drc.json')}}
$calls += @{tool='run_design_review'; arguments=@{schematic=(Join-Path $hw 'ElevatorLift.kicad_sch');board=(Join-Path $hw 'ElevatorLift.kicad_pcb');severity_filter='warning'}}
$result = & (Join-Path $hw 'tools/konnect-batch.ps1') -CallsJson ($calls | ConvertTo-Json -Depth 12 -Compress) -Toolsets @('sch_analysis','sch_components','sch_export','pcb_export','design_review')
# Generated diagnostic evidence, not KiCad source modifications.
$result | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'konnect-results.json') -Encoding utf8
$parsed = $result | ConvertFrom-Json
for ($i=0; $i -lt $parsed.Count; $i++) {
  [pscustomobject]@{tool=$parsed[$i].tool;arguments=$calls[$i].arguments;isError=$parsed[$i].isError;resultChars=$parsed[$i].result.Length} | ConvertTo-Json -Compress
}
