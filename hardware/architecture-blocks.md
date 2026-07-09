# Hardware Architecture Blocks

## Root Signals

- `AC_L`, `AC_N`, `PE/chassis`
- `DC_12V`, `3V3`
- `VFD_TX`, `VFD_RX`, optional `VFD_ENABLE`
- `ENC_A_RAW`, `ENC_B_RAW`, `ENC_A_CLEAN`, `ENC_B_CLEAN`
- `SPI_SCK`, `SPI_MOSI`, `SPI_MISO`, chip selects for counter/MRAM/flash
- `I2C_SCL`, `I2C_SDA` for RTC and future low-speed peripherals
- `RF_DATA`
- `SAFETY_MON`, `HOME`, `LIMIT_UP`, `LIMIT_DOWN`
- `LIGHT_OUT`

## Power Block

The power page should keep mains and SELV circuitry visibly separated. Start with a 120 VAC input connector, fuse/surge/filtering placeholders, and a RECOM `RAC10-12SK/277` 12 V supply footprint. Generate 3.3 V from 12 V with a buck regulator.

## MCU Block

The MCU page should be centered on ESP32-S3-WROOM-1U or a compatible external-antenna module. Break out enough spare GPIO for revision-A changes. Include USB/debug access, boot/reset handling, and a status LED.

## VFD Interface Block

The VFD page should support standard 9600 baud UART framing into the VFD opto-isolated serial input. Use a transistor/MOSFET current driver for the VFD RX/opto input, a protected receive path for VFD TX, test points, and a possible hardwired stop/enable output if the VFD supports it.

## Encoder/Counter Block

The encoder page should support open-collector Hall outputs with 5 V pullups, starting at the SKF recommended 270 ohm value, plus protection, optional filtering, Schmitt cleanup, and LS7366R SPI quadrature counting. The schematic must resolve 5 V encoder logic versus 3.3 V MCU SPI with a level-shift or translated counter path.

## Storage Block

The storage page should include SPI MRAM as mandatory. Optional QSPI/SPI NOR flash or microSD should be a stuffing option, not required for the minimum controller.

## RF/Input Block

The RF/input page must support the RXM-418-LR receiver path and protected inputs for buttons/home/limits/safety monitoring. Keep safety monitoring distinct from hardwired safety authority.

## Output Block

Start with a protected 12 V light-control output sized for about 750 mA, likely a MOSFET switch. Leave a low-voltage auxiliary output header if future external relays or SSRs are needed.
