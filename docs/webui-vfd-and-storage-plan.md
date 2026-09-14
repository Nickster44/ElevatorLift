# WebUI And Storage Direction

Reviewed 2026-09-14. Design intent is separate from the inhibited target baseline.

## Current Implementation

- PM004MNIATR provides 4 Mbit (524,288 bytes). The MRAM journal reserves 768 event
  slots of 576 bytes, using bounded JSON payloads and integrity metadata. Earlier
  64-byte binary-record estimates are not implemented capacity.
- MRAM holds snapshots, configuration and fault/event records. Snapshots cannot
  prove incremental position continuity after reset.
- Station credentials use NVS, not MRAM. API credentials are build configuration.
- Web assets use internal flash/LittleFS. Rev A has no SD or separate QSPI NOR.
  OTA-sized partitions exist, but no updater or atomic asset rollback.
- Diagnostic VFD reads use a timestamped RAM cache. Guarded write/readback and
  persistent cache logic are isolated, not target write capabilities.

Use the [firmware guide](../firmware/README.md) for exact storage addresses and
[delivery contract](../webapp/docs/embedded-delivery.md) for asset budgets.

## Remaining Product Work

Target floor programming, explicit calibration, guarded VFD writes, RF learning,
backup/restore and paginated log export remain unfinished. Host models do not
establish target capabilities. RF nicknames require learned slot plus association
epoch, never TX_ID alone. Decoder address memory cannot be enumerated or backed up.
Current/overload settings are not certified weight measurements.

Dense binary logging remains future work. Reconsider external/removable storage
only if measured retention requirements exceed the current board's capacity.

Canonical requirements and workflows:

- [Controller requirements](new-controller-requirements.md)
- [Motion and calibration](motion-control-and-calibration.md)
- [API contract](../webapp/docs/api-contract.md)
- [Firmware/WebUI gaps](../webapp/docs/firmware-integration-gaps.md)
- [Home automation plan](home-automation-integration.md)
