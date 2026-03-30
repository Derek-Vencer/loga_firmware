# LOGA 
## Version & Date

V0014.18 - 2026/03/27

## New Feature
Two new RGB animation
* FN + CTRL + 7 (Starlight)
* FN + CTRL + 8 (Ripple)

KEYCODE
* FN + A (Windows layout)
* FN + S (Mac Layout)

mac function key FN + (1-12)

API
* 2.45 Set Lighting Value(0x020C)
* 2.46 Get Lighting Value(0x028C)

Set payload

| Offset | Value      | Comment       |
|--------|------------|---------------|
| 1      | 0x00-0x09  | 0x00(Lowest)  |
|        |            | 0x09(Highest) |
| 3      | 0x00-0x05  | 0x00(Fastest) |
|        |            | 0x04(Slowest) |

## Bug Fixes
set color
## Known Issue