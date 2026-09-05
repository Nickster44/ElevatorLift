# Hardware Architecture Blocks

## Root Signals

- `AC_L`, `AC_N`, `PE/chassis`
- `DC_12V`, `5V`, `3V3`
- `VFD_TX`, `VFD_RX`, optional `VFD_ENABLE`
- `ENC_A_RAW`, `ENC_B_RAW`, `ENC_A_CLEAN`, `ENC_B_CLEAN`
- `SPI_SCK`, `SPI_MOSI`, `SPI_MISO`, chip selects for counter/MRAM/flash
- `I2C_SCL`, `I2C_SDA` for RTC and future low-speed peripherals
- `RF_D0` through `RF_D4`, `RF_TX_ID`, `RF_LEARN`, `RF_MODE_IND`
- `SAFETY_MON`, `HOME`, `LIMIT_UP`, `LIMIT_DOWN`
- `LIGHT_OUT`

## Power Block

The power page should keep mains and SELV circuitry visibly separated. Use a 120 VAC + neutral input connector, fuse and MOV, and a RECOM `RAC10-12SK/277` 12 V supply. The current Rev-A design then uses an `R-78E5.0-0.5` from 12 V to 5 V and an AP63203 stage from 12 V to 3.3 V. The fact that the legacy controller fed the R-78E from 24 V is historical evidence only; the selected converter supports the new 12 V input arrangement.

## MCU Block

The MCU page is centered on the exact `ESP32-S3-WROOM-1U-N16R8` external-antenna module. Break out enough spare GPIO for revision-A changes. Include native USB/debug access, boot/reset handling, and a status LED.

## VFD Interface Block

The VFD page should support standard 9600 baud UART framing. The observed legacy implementation uses a TI `TXS0104E` to translate the MCU-side signals between 3.3 V and 5 V, feeding a four-pin VFD header with no discrete driver observed. Capture that reference topology with OE default-low, test pads, and a configurable protection/series-resistor option. Do not connect it to the lift until the header pinout, common/reference, idle levels, and fault behavior are bench-verified. Include a possible hardwired stop/enable output if the VFD supports it.

## Encoder/Counter Block

The observed legacy encoder page uses a 12 V supply, 270 ohm pullups on open-collector A/B, 270 ohm series resistors, and a `TLP291-4` photocoupler. Preserve this as the reference interface pending signal tracing; the additional optocoupler and HC74 behavior is unknown. The new schematic must resolve optocoupler output conditioning and LS7366R-compatible logic levels after measuring encoder current and maximum pulse rate.

## Storage Block

The storage page includes SPI MRAM as mandatory and a low-power RTC for timestamps when NTP is unavailable. The N16R8 module's internal 16 MB flash carries Rev-A WebUI assets and OTA staging; external QSPI/SPI NOR or microSD is deferred unless measured capacity later proves insufficient.

## RF/Input Block

The RF/input page must support the RXM-418-LR receiver feeding a `LICAL-DEC-MS001` decoder, whether on a qualified harvested module/daughterboard or an equivalent new circuit. Route all five decoded button outputs, the decoder `TX_ID` output, and `MODE_IND` to protected MCU inputs. `LEARN` is not available on the legacy header, so provide a dedicated wired pad/header to its physical-button node, a safe-default-low MCU output, and local service access. The five remote buttons map to Floor 1, Floor 2, Floor 3, Stop, and Light toggle. Keep safety monitoring distinct from hardwired safety authority.

The decoder can learn up to 40 transmitter addresses. Firmware must be able to request the 17-second Learn Mode, observe `MODE_IND`, and deliberately hold `LEARN` high for the decoder's 10-second erase-all operation. The hardware does not support deleting one learned address at a time.

Provide a separate cabinet-local keyed/service input and continuous hold-to-run input if restricted safety-loop recovery is retained. These inputs may only authorize firmware to disregard a diagnosed *monitored* safety channel in service mode; they must not bypass the emergency stop, hardwired final limits, watchdog/VFD enable chain, or other independent removal of motion authority.

## Output Block

Start with a protected 12 V light-control output sized for about 750 mA, likely a MOSFET switch. Leave a low-voltage auxiliary output header if future external relays or SSRs are needed.
