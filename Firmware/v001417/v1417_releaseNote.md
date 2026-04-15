# LOGA 
## Version & Date

V0014.17 - 2026/03/20

## New Feature
Two new RGB animation
* Wheel
* Reactive

KEYCODE for RGB
* FN + CTRL + 6
* FN + CTRL + 9

USB Polling rate change
* get usb polling rate(0x0190)
* set usb polling rate(0x0110)

Command payload

| Offset | Value | Comment |
|--------|-------|---------|
| 0      | 0x00  | 8000Hz  |
|        | 0x01  | 4000Hz  |
|        | 0x02  | 2000Hz  |
|        | 0x03  | 1000Hz  |

Acknowledgement payload

| Offset | Value | Comment |
|--------|-------|---------|
| 0      | 0x00  | Success |
|        | 0xff  | Fail    |

## Bug Fixes
KEYCODE for RGB
* FN + CTRL + 1
* FN + CTRL + 2
* FN + CTRL + 3
* FN + CTRL + 4
* FN + CTRL + 5

Factory Reset
* FN + ESC for 3s(in second profile)
## Known Issue