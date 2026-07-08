# Hardware / KiCad Workspace

This folder is the hardware design workspace. The current files are a KiCad scaffold and planning notes, not a completed schematic.

## Files

| File | Purpose |
| --- | --- |
| `ElevatorLift.kicad_pro` | KiCad project shell |
| `ElevatorLift.kicad_sch` | Root schematic shell with block-level text notes |
| `ElevatorLift.kicad_pcb` | Empty PCB shell |
| `architecture-blocks.md` | Schematic page/block plan |

## Schematic Page Plan

1. Root block diagram and connector index.
2. AC input and isolated DC power.
3. ESP32-S3 module, USB/debug, boot/reset, status indicators.
4. VFD serial interface and hardware stop/enable path.
5. Encoder conditioning and quadrature counter.
6. MRAM, optional flash/SD, RTC.
7. RF receiver and user inputs.
8. Light output and any retained auxiliary outputs.

## Before Schematic Capture

Resolve the open questions in `docs/hardware-requirements-matrix.md`, especially VFD serial electrical levels, field input voltages, VFD-box mounting space, encoder supply/pullups, and light load details.

