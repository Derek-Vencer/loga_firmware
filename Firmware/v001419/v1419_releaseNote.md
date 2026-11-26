# LOGA 
## Version & Date

V0014.19 - 2026/04/08

## New Feature

API
* 2.35 Set Macro Data (0x0205)
* 2.36 Get Macro Data (0x0285)
* 2.37 Clear Macro (0x0206)

Set Macro Data payload

| Offset | Value      | Comment       |
|--------|------------|---------------|
| 0      | 0x00-0x09  | macro index   |
| 1      | 0x00-0xB4  | step index    |
| 2      | 0x00-0xFF  | keycode low   |
| 3      | 0x00-0xFF  | keycode high  | 
| 4      | 0x00       | delay_type    |
| 5      | 0x00-0xFF  | delay_ms low  |
| 6      | 0x00-0xFF  | delay_ms high |
| 7      | 0x01-0x02  | action(0x01 press 0x02 release)|

Get Macro Data payload
 * sending response payload per packet:

| Offset | Value      | Comment       |
|--------|------------|---------------|
| 0      | 0x00-0x09  | macro index   |
| 1      | 0x00-0xB4  | step index    |
| 2      | 0x00-0xFF  | keycode low   |
| 3      | 0x00-0xFF  | keycode high  | 
| 4      | 0x00       | delay_type    |
| 5      | 0x00-0xFF  | delay_ms low  |
| 6      | 0x00-0xFF  | delay_ms high |
| 7      | 0x01-0x02  | action(0x01 press 0x02 release)|

 * final response payload:
 
| Offset | Value      | Comment       |
|--------|------------|---------------|
| 0      | 0x00-0x09  | macro index   |
| 1      | 0x00-0xB4  | step index    |


## Bug Fixes

## Known Issue