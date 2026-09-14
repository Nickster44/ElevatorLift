# Documentation Guide

Use current guidance for design work; dated review reports retain evidence and
must not be treated as a live pin map, BOM or readiness approval.

## Current design and implementation

- [Repository overview](../README.md) and [hardware workspace](../hardware/README.md).
- [Requirements](new-controller-requirements.md) and
  [hardware requirements matrix](hardware-requirements-matrix.md): intended capabilities.
- [Hardware selection](hardware-selection.md) and
  [capture register](schematic-capture-readiness.md): selected, captured blocks.
- [Hardware dependencies](hardware-dependency-handoff.md): unresolved qualification gates.
- [Software checklist](software-integration-checklist.md),
  [firmware guide](../firmware/README.md) and [API contract](../webapp/docs/api-contract.md):
  actual software support and remaining integration work.
- [Library requests](component-library-request.md),
  [sourcing references](jlcpcb-lcsc-sourcing.md), and
  [library portability](snapeda-library-import.md).

## Evidence retained deliberately

- [Field observations](field-verification-log.md) distinguish old-board observations
  from decisions for the new controller.
- [External manual-control box review](reviews/2026-09-13-manual-control-box.md)
  contains the missing wiring/brake evidence checklist. Its dated software snapshot
  is superseded by the current handoff, not its unresolved physical questions.
- [Pin/comms/four-layer report](reviews/2026-09-13-pin-comms-fourlayer.md)
  records the hardware change and connectivity checks; firmware subsequently adopted it.
- [Original integration review](reviews/2026-09-10-integration/README.md) and
  [old-system review](old-system-review.md) are historical, not current implementation.

Do not delete an unresolved safety or mechanical question merely because it is
old. Close it only with evidence. Component candidates and duplicate planning
text no longer applicable to Rev A have been removed from the current guides;
previous versions remain recoverable through Git history.
