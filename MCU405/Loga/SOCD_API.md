# SOCD API Notes

## Overview

- SOCD slot count: `SOCD_SLOT_COUNT = 10`
- SOCD config storage:
  - Runtime RAM: `socd_slots[]`
  - EEPROM base: `EECONFIG_SOCD_ADDR`
- Global SOCD on/off:
  - Runtime flag: `isSOCD`
  - EEPROM flag: `IS_SOCD_ON_ADD`
- API command id:
  - `SET SOCD`: `0x010B`
  - `GET SOCD`: `0x018B`

## SOCD Mode

- `0`: `SOCD_PRIORITY_LAST_INPUT`
- `1`: `SOCD_PRIORITY_PRIMARY`
- `2`: `SOCD_PRIORITY_SECONDARY`
- `3`: `SOCD_PRIORITY_NEUTRAL`
- `4`: `SOCD_DISABLE`

## Slot Data Format

Current SOCD slot data uses this 8-byte format:

```text
buf[0] = a_layer
buf[1] = a_row
buf[2] = a_col
buf[3] = b_layer
buf[4] = b_row
buf[5] = b_col
buf[6] = mode
buf[7] = slot_index
```

The firmware will resolve `(layer,row,col)` into the current EEPROM keymap keycode and store:

- `kc_a`
- `kc_b`
- `a_layer/a_row/a_col`
- `b_layer/b_row/b_col`
- `mode`

## SET

### API

- Command: `ione_api_cmd_set_snap_tap_SOCD_Binding`
- Command id: `0x010B`
- Payload length: `8`

### Payload

```text
[a_layer, a_row, a_col, b_layer, b_row, b_col, mode, slot_index]
```

### Firmware Path

- `via.c`
  - `ione_api_cmd_set_snap_tap_SOCD_Binding`
  - calls `socd_apply_report()`
- `socd.c`
  - `socd_apply_report()`
  - `eeconfig_write_socd()`
  - `socd_apply_config()`

### Behavior

- The target slot is selected by `buf[7]`
- The keycode is resolved from the current EEPROM keymap, not from static flash keymap
- After write, the slot is immediately applied into runtime

## GET

### API

- Command: `ione_api_cmd_get_snap_tap_SOCD_Binding`
- Command id: `0x018B`

### Supported Modes

#### 1. Get all SOCD slots

- Request: `payload_length == 0`
- Response: firmware sends one packet per slot
- Current behavior: all `SOCD_SLOT_COUNT` slots are iterated and returned

Each returned packet payload is:

```text
[a_layer, a_row, a_col, b_layer, b_row, b_col, mode, slot_index]
```

#### 2. Get one SOCD slot

- Request: `payload_length == 1`
- Request payload:

```text
[slot_index]
```

- Response payload:

```text
[a_layer, a_row, a_col, b_layer, b_row, b_col, mode, slot_index]
```

### Firmware Path

- `via.c`
  - `ione_api_cmd_get_snap_tap_SOCD_Binding`
- `socd.c`
  - `socd_fill_report()`

## DELETE

## Current Status

There is currently **no dedicated single-slot delete API** for SOCD.

### What really exists now

#### A. Clear all SOCD slots

This is the only real delete path in current firmware.

- Function: `eeconfig_socd_clear()`
- Behavior:
  - clear all SOCD EEPROM slots
  - clear all SOCD runtime state
  - unregister active SOCD output key if needed

### Where `eeconfig_socd_clear()` is used

- Factory reset API:
  - `ione_api_cmd_set_factory_reset` (`0x0201`)
  - `ione_api_cmd_factory_reset_and_system_reset` (`0x020A`)
- Keyboard side long-hold reset:
  - `EECLR` hold for 3 seconds

### Result

- All SOCD slots are deleted together
- Not per-slot

#### B. Set mode to `SOCD_DISABLE`

This is **not a true delete**.

Reason:

- `socd_apply_report()` always writes:
  - `enable = 1`
- `socd_find_slot_by_keycode()` only checks:
  - `enable == 1`
- So a slot with `mode = SOCD_DISABLE` is still considered an active slot match

### Side effect

If a slot is written with:

- same key pair
- `mode = SOCD_DISABLE`

then those keys may still be claimed by SOCD, but output becomes `0`.

This means:

- it behaves like "matched but no output"
- not "slot removed"

So:

- `SOCD_DISABLE` can be treated as logical off for output
- but it is **not** a safe replacement for delete

## Recommended Delete Rule

### If you want to delete all SOCD

Use:

- factory reset API, or
- `eeconfig_socd_clear()`

### If you want to delete one SOCD slot

Current firmware does **not** have a proper per-slot delete command.

If single-slot delete is needed, a correct implementation should do one of these:

1. Write the slot as:
   - `enable = 0`
   - clear keycode / layer / row / col
   - `mode = SOCD_DISABLE`
2. Add a dedicated SOCD delete API:
   - input: `slot_index`
   - action: clear only that slot in EEPROM and RAM

## Global SOCD Enable/Disable

This is different from slot config.

- Runtime flag: `isSOCD`
- Stored in EEPROM: `IS_SOCD_ON_ADD`

This global switch controls whether matrix processing will hand key events to SOCD.

### Toggle key

- Keycode: `KC_SOCD_TOGG`

### Behavior

- toggle `isSOCD`
- write result back to EEPROM
- survives reboot

## Summary

- `SET` exists and writes one slot
- `GET` exists and now supports:
  - one slot
  - all slots
- Real `DELETE` currently exists only for:
  - clear all slots
- Per-slot delete does not exist yet
- `mode = SOCD_DISABLE` is not the same as deleting a slot

## Simplest Test Steps

This is the shortest end-to-end smoke test for current firmware.

### Goal

Verify:

- `SET` can write one SOCD slot
- `GET` can read back the same slot
- SOCD behavior really changes key output
- `GET all` returns all current slots

### Step 1: Make sure global SOCD is ON

- Use `KC_SOCD_TOGG` once if needed
- If SOCD was already ON, do not toggle it again

### Step 2: Pick one easy key pair

Use one pair you can test by hand, for example:

- `A`
- `D`

Find their:

- `layer`
- `row`
- `col`

### Step 3: Write slot 0

Send `SET SOCD`:

- Command id: `0x010B`
- Payload:

```text
[a_layer, a_row, a_col, b_layer, b_row, b_col, 3, 0]
```

Meaning:

- `mode = 3` -> `SOCD_PRIORITY_NEUTRAL`
- `slot_index = 0`

### Step 4: Read back slot 0

Send `GET SOCD` single slot:

- Command id: `0x018B`
- Payload:

```text
[0]
```

Expected:

- Returned 8 bytes match the slot you just wrote

```text
[a_layer, a_row, a_col, b_layer, b_row, b_col, 3, 0]
```

### Step 5: Do the real keyboard test

Physically press both keys together:

- press `A` + `D`

Expected with `SOCD_PRIORITY_NEUTRAL`:

- output becomes neutral
- usually means neither direction is sent

Then test one-by-one:

- press only `A` -> only `A` output
- press only `D` -> only `D` output

### Step 6: Read all slots

Send `GET SOCD` with empty payload:

- Command id: `0x018B`
- Payload length: `0`

Expected:

- firmware sends one packet per slot
- current firmware should return `10` packets because `SOCD_SLOT_COUNT = 10`

### Step 7: Simplest delete test

Current firmware has no true single-slot delete.

If you want to verify delete behavior, use:

- factory reset API, or
- hold `EECLR` for 3 seconds

Then run `GET SOCD` again and confirm all slots are cleared.
