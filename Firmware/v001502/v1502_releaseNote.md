# LOGA 
## Version & Date

V0015.02 - 2026/05/21

## New Feature

API
* 2.38 Set SOCD Binding (0x010B)
* 2.39 Get SOCD Binding (0x018B)
* 2.40 Lighting Brightness/Speed Percentage Range Update
* SOCD slot count updated to `10`

Set SOCD Binding payload

| Offset | Value      | Comment       |
|--------|------------|---------------|
| 0      | 0x00-0xFF  | a_layer       |
| 1      | 0x00-0xFF  | a_row         |
| 2      | 0x00-0xFF  | a_col         |
| 3      | 0x00-0xFF  | b_layer       |
| 4      | 0x00-0xFF  | b_row         |
| 5      | 0x00-0xFF  | b_col         |
| 6      | 0x00-0x04  | mode          |
| 7      | 0x00-0x09  | slot index    |

Get SOCD Binding payload
 * single slot request:

| Offset | Value      | Comment       |
|--------|------------|---------------|
| 0      | 0x00-0x09  | slot index    |

 * response payload per slot:

| Offset | Value      | Comment       |
|--------|------------|---------------|
| 0      | 0x00-0xFF  | a_layer       |
| 1      | 0x00-0xFF  | a_row         |
| 2      | 0x00-0xFF  | a_col         |
| 3      | 0x00-0xFF  | b_layer       |
| 4      | 0x00-0xFF  | b_row         |
| 5      | 0x00-0xFF  | b_col         |
| 6      | 0x00-0x04  | mode          |
| 7      | 0x00-0x09  | slot index    |

Lighting Value update
* Brightness API range changed to `0x00-0x64`
* Speed API range changed to `0x00-0x64`
* Brightness `0x00` now behaves as LED off

## Bug Fixes

* Fixed SOCD key resolution to use current EEPROM/VIA keymap instead of static flash keymap
* Fixed SOCD global enable state default to ON and keep state after reboot
* Fixed SOCD GET API to return all configured slots when payload length is 0
* Fixed SOCD EEPROM clear/init handling for invalid `0xFF` data
* Fixed RGB brightness `0` still showing light; now enters off mode correctly
* Updated internal LED brightness/speed handling to match percentage-based API behavior


## Known Issue

* SOCD does not currently support dedicated single-slot delete API
* `SOCD_DISABLE` is not equivalent to removing a SOCD slot
