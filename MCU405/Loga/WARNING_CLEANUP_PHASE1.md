# Warning cleanup - Phase 1

Modified files:
- matrix.c
- game_pad.c / game_pad.h
- dynamic_macro.c
- macro_engine.c
- led/led_function.c / led/led_function.h
- led/led_effect.c
- led/driver/led_driver.c / led/driver/led_driver.h

Main changes:
1. Added missing local headers/prototypes to prevent implicit declarations and LTO type mismatches.
2. Corrected SPI callback signature to `void callback(SPIDriver *)`.
3. Corrected `LED_SpeedChange` declaration to match its `int8_t` implementation.
4. Added `rapidTriggerProcess` and `send_gamepad` declarations before use.
5. Corrected Rapid Trigger structure byte serialization with an explicit `uint8_t *` cast.
6. Changed RGB value adjustment key handling to `register_code16` / `unregister_code16`.
7. Removed several unused variables and marked thread parameters unused.
8. Eliminated compile-time division-by-zero expressions.
9. Removed an impossible unsigned `< 0` comparison.
10. Added the missing success return in `setRunMode`.
11. Fixed the LED preset index increment expression that had undefined sequence-point behavior.

Not included because the uploaded archive does not contain `../../keyboard_core` sources:
- report.h struct/comment warnings
- keymap_in_eeprom.c EEPROM pointer and return-type warnings
- action.c keycode-width warnings
- hook.c / encoder.c / i2c_master.c non-void return warnings
- usb/core header prototype cleanup

Build verification must be performed in the original directory where `../../keyboard_core` exists.
