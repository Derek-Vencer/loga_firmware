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

### API

- Command: `ione_api_cmd_delete_snap_tap_SOCD_Binding`
- Command id: `0x019B`

### Supported Modes

#### 1. Delete all SOCD slots

- Request:
  - `payload_length == 0`, or
  - `payload_length == 1` with payload `[0xFF]`
- Behavior:
  - calls `eeconfig_socd_clear()`
  - clears all SOCD EEPROM slots
  - clears all SOCD runtime state
  - unregisters active SOCD output key if needed

#### 2. Delete one SOCD slot

- Request: `payload_length == 1`
- Request payload:

```text
[slot_index]
```

- Behavior:
  - calls `eeconfig_socd_clear_slot(slot_index)`
  - clears only that slot in EEPROM and RAM
  - unregisters the slot's active SOCD output key if needed

### Notes

- Writing `mode = SOCD_DISABLE` is still not the same as delete
- Real delete now uses:
  - `eeconfig_socd_clear()`
  - `eeconfig_socd_clear_slot()`

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
- `DELETE` now supports:
  - clear all slots
  - clear one slot
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

### Step 7: Delete test

Delete slot 0:

- Command id: `0x019B`
- Payload:

```text
[0]
```

Delete all slots:

- Command id: `0x019B`
- Payload:

```text
[0xFF]
```

Then run `GET SOCD` again and confirm the target slot, or all slots, are cleared.
