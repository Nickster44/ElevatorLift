# Hardware Architecture Blocks

## Root Signals

- `AC_L`, `AC_N`, `PE/chassis`
- `DC_5V` or `DC_12V`, `3V3`
- `VFD_TX`, `VFD_RX`, optional `VFD_ENABLE`
- `ENC_A_RAW`, `ENC_B_RAW`, `ENC_A_CLEAN`, `ENC_B_CLEAN`
- `SPI_SCK`, `SPI_MOSI`, `SPI_MISO`, chip selects for counter/MRAM/flash
- `I2C_SCL`, `I2C_SDA` for RTC and future low-speed peripherals
- `RF_DATA`
- `SAFETY_MON`, `HOME`, `LIMIT_UP`, `LIMIT_DOWN`
- `LIGHT_OUT`

## Power Block

The power page should keep mains and SELV circuitry visibly separated. Start with an isolated PCB-mount AC/DC module and add fuse, surge, and filtering options after the actual VFD cabinet power tap is confirmed.

## MCU Block

The MCU page should be centered on ESP32-S3-WROOM-1U or a compatible external-antenna module. Break out enough spare GPIO for revision-A changes. Include USB/debug access, boot/reset handling, and a status LED.

## VFD Interface Block

The VFD page should support protected UART and leave room for digital isolation until the VFD serial electrical layer is confirmed. Include test points and a possible hardwired stop/enable output if the VFD supports it.

## Encoder/Counter Block

The encoder page should support open-collector Hall outputs with configurable pullups, protection, optional filtering, Schmitt cleanup, and LS7366R SPI quadrature counting.

## Storage Block

The storage page should include SPI MRAM as mandatory. Optional QSPI/SPI NOR flash or microSD should be a stuffing option, not required for the minimum controller.

## RF/Input Block

The RF/input page should support the RXM-418-LR receiver path and protected inputs for buttons/home/limits/safety monitoring. Keep safety monitoring distinct from hardwired safety authority.

## Output Block

Start with a light-control output sized to the real load. Leave a low-voltage auxiliary output header if future external relays or SSRs are needed.

