# Hardware / KiCad Workspace

This folder is the hardware design workspace. The current files are a KiCad 10 scaffold and planning notes, not a completed schematic.

## Files

| File | Purpose |
| --- | --- |
| `ElevatorLift.kicad_pro` | KiCad project shell |
| `ElevatorLift.kicad_sch` | Root schematic shell with block-level text notes |
| `ElevatorLift.kicad_pcb` | PCB shell with provisional 3.5 in x 3.5 in outline and corner mounting holes |
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

Use `docs/hardware-requirements-matrix.md` and the main README physical verification checklist as the source of truth. The current schematic assumptions are:

- Target PCB outline: provisional 3.5 in x 3.5 in.
- Main supply: 120 VAC input to RECOM `RAC10-12SK/277` 12 V module.
- Logic supply: 3.3 V buck from 12 V.
- VFD serial: 9600 baud UART into opto-isolated VFD input with a transistor/MOSFET current driver.
- Encoder: SKF-style 5 V open-collector quadrature, starting with 270 ohm pullups and protected/conditioned inputs.
- RF: RXM-418-LR receiver path is mandatory.
- Light output: protected 12 V MOSFET output, pending final load verification.

Still confirm field input voltages, exact VFD-box mounting geometry, VFD opto input current, light inrush/load type, and old-board IC markings before final schematic release.
