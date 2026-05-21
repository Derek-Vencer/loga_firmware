# Macro key Test Process
## First step
API set key mapping(0x0102)

Please ensure you can change the key code.

> for example change ESC to A

`0102 0000000004`

> If success change ESC to MACRO 1

`0102 0000005F17`

| Description  | Keycode    |
|--------------|------------|
| Macro 1      | 0x5F17     |
| Macro 2      | 0x5F18     |
| Macro 3      | 0x5F19     |
| Macro 4      | 0x5F1A     |
| Macro 5      | 0x5F1B     |
| Macro 6      | 0x5F1C     |
| Macro 7      | 0x5F1D     |
| Macro 8      | 0x5F1E     |
| Macro 9      | 0x5F1F     |
| Macro 10     | 0x5F20     |

## Second step

API set Macro Data(0x0205)

> Set macro key step one press B

`0205 0000050000000001`


> Set macro key step two, release B after 10 milliseconds

`0205 00010500000A0002`

If kecode is not working properly, make sure to clear the macros.

> for example clear Macro 1

`0206 00`