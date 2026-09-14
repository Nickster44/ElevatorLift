# Hardware Architecture Blocks

## Root Signals

- `AC_L`, `AC_N`, `PE/chassis`
- `DC_12V`, `5V`, `3V3`
- `VFD_TX_3V3`, `VFD_RX_3V3`, `VFD_COMMS_ENABLE` (GPIO42, UART OE only)
- `ENC_A_RAW`, `ENC_B_RAW`, `ENC_A_CLEAN`, `ENC_B_CLEAN`
- `SPI_SCK`, `SPI_MOSI`, `SPI_MISO`, chip selects for counter/MRAM; no external asset-flash IC
- `I2C_SCL`, `I2C_SDA` for RTC and future low-speed peripherals
- `RF_D0` through `RF_D4`, `RF_TX_ID`, `RF_LEARN`, `RF_MODE_IND`
- `SAFETY_MON`, `HOME`, `LIMIT_UP`, `LIMIT_DOWN`
- `LIGHT_OUT`

## Power Block

The power page should keep mains and SELV circuitry visibly separated. Use a 120 VAC + neutral input connector, fuse and MOV, and a RECOM `RAC10-12SK/277` 12 V supply. The current Rev-A design then uses an `R-78E5.0-0.5` from 12 V to 5 V and an AP63203 stage from 12 V to 3.3 V. The fact that the legacy controller fed the R-78E from 24 V is historical evidence only; the selected converter supports the new 12 V input arrangement.

## MCU Block

The MCU page is centered on the exact `ESP32-S3-WROOM-1U-N16R8` external-antenna module. Native USB, boot/reset handling and a status LED are captured. GPIO35-37 are reserved for octal PSRAM; GPIO43/44/3 serve limits/service key. J11 pins 3/4 are NC.

## VFD Interface Block

The VFD page supports the existing UART translation topology using TI `TXS0104E` between 3.3 V and 5 V. OE is default-low and now controlled only by `VFD_COMMS_ENABLE` on GPIO42. The MCU-driven RUN collector has been removed. Preserve the independent external hardwired safety loop; verify VFD configuration cannot bypass it through serial commands. The established J24 order is +5V, GND, VFD RX, VFD TX. Header reference, idle levels, protocol configuration and fault behavior still require bench verification before connection to the lift.

## Encoder/Counter Block

The captured encoder path uses 12 V field inputs, 2.2 kΩ LED resistors,
TLP291-4, 74HC14D conditioning and LS7366R-S with a 4 MHz clock. The old
270 Ω resistor path and unexplained HC74 are historical references only.
Actual encoder current, phase and maximum pulse rate remain unverified.

## Storage Block

The storage page includes SPI MRAM as mandatory and a low-power RTC for timestamps when NTP is unavailable. The N16R8 module's internal 16 MB flash carries Rev-A WebUI assets and OTA staging; external QSPI/SPI NOR or microSD is deferred unless measured capacity later proves insufficient.

## RF/Input Block

The RF/input page must support the RXM-418-LR receiver feeding a `LICAL-DEC-MS001` decoder, whether on a qualified harvested module/daughterboard or an equivalent new circuit. Route all five decoded button outputs, the decoder `TX_ID` output, and `MODE_IND` to protected MCU inputs. `LEARN` is not available on the legacy header, so provide a dedicated wired pad/header to its physical-button node, a safe-default-low MCU output, and local service access. The owner-specified software map is D0=Floor 1, D1=Light, D2=Floor 3, D3=Floor 2, D4=Stop. Physical mapping remains unverified. Keep safety monitoring distinct from hardwired safety authority.

The decoder can learn up to 40 transmitter addresses. Firmware must be able to request the 17-second Learn Mode, observe `MODE_IND`, and deliberately hold `LEARN` high for the decoder's 10-second erase-all operation. The hardware does not support deleting one learned address at a time.

J43/J44 provide active-low key, hold and direction inputs with shared controller
GND. The proposed external box owns the keyed override; its contact topology and
independent drive/brake stopping authority are not verified. No monitored GPIO
is proof of a safety-rated enabling circuit. See the
[external-box evidence review](../docs/reviews/2026-09-13-manual-control-box.md).

## Output Block

The protected 12 V light-control output remains. The optional auxiliary output
and its header were removed September 12–13, 2026, freeing GPIO3 for SERVICE_KEY.
The MCU-driven VFD RUN output was also removed; the independent external
hardwired safety chain remains the required, unverified RUN authority. See the current hardware handoff
for the GPIO43/44 limit reassignment, reset assumptions and four-layer plan.
