#include <stdint.h>
#include "keyboard.h"
#include "action.h"
#include "util.h"
#include "action_layer.h"
#include "hook.h"

#ifdef DEBUG_ACTION
#include "debug.h"
#else
#include "nodebug.h"
#endif


/* 
 * Default Layer State
 */
uint32_t default_layer_state = 0;

static void default_layer_state_set(uint32_t state)
{
    debug("default_layer_state: ");
    default_layer_debug(); debug(" to ");
    default_layer_state = state;
    hook_default_layer_change(default_layer_state);
    default_layer_debug(); debug("\n");
#ifdef LEDMAP_ENABLE
    default_layer_state_change(state);
#endif
    clear_keyboard_but_mods(); // To avoid stuck keys
}

void default_layer_debug(void)
{
    dprintf("%08lX(%u)", default_layer_state, biton32(default_layer_state));
}

void default_layer_set(uint32_t state)
{
    default_layer_state_set(state);
}

#ifndef NO_ACTION_LAYER
void default_layer_or(uint32_t state)
{
    default_layer_state_set(default_layer_state | state);
}
void default_layer_and(uint32_t state)
{
    default_layer_state_set(default_layer_state & state);
}
void default_layer_xor(uint32_t state)
{
    default_layer_state_set(default_layer_state ^ state);
}
#endif


#ifndef NO_ACTION_LAYER
/* 
 * Keymap Layer State
 */
uint32_t layer_state = 0;

void layer_state_set(uint32_t state)
{
    dprint("layer_state: ");
    layer_debug(); dprint(" to ");
    layer_state = state;
    hook_layer_change(layer_state);
    layer_debug(); dprintln();
#ifdef LEDMAP_ENABLE
    layer_state_change(state);
#endif
    clear_keyboard_but_mods(); // To avoid stuck keys
}

void layer_clear(void)
{
    layer_state_set(0);
}

void layer_move(uint8_t layer)
{
    layer_state_set(1UL<<layer);
}

void layer_on(uint8_t layer)
{
    layer_state_set(layer_state | (1UL<<layer));
}

void layer_off(uint8_t layer)
{
    layer_state_set(layer_state & ~(1UL<<layer));
}

void layer_invert(uint8_t layer)
{
    layer_state_set(layer_state ^ (1UL<<layer));
}

void layer_or(uint32_t state)
{
    layer_state_set(layer_state | state);
}
void layer_and(uint32_t state)
{
    layer_state_set(layer_state & state);
}
void layer_xor(uint32_t state)
{
    layer_state_set(layer_state ^ state);
}

void layer_debug(void)
{
    dprintf("%08lX(%u)", layer_state, biton32(layer_state));
}
#endif

#if !defined(NO_ACTION_LAYER) && !defined(STRICT_LAYER_RELEASE)
/** \brief source layer cache
 */

uint8_t source_layers_cache[(MATRIX_ROWS * MATRIX_COLS + 7) / 8][MAX_LAYER_BITS] = {{0}};

/** \brief update source layers cache
 *
 * Updates the cached keys when changing layers
 */
void update_source_layers_cache(keypos_t key, uint8_t layer) {
    const uint8_t key_number  = key.col + (key.row * MATRIX_COLS);
    const uint8_t storage_row = key_number / 8;
    const uint8_t storage_bit = key_number % 8;

    for (uint8_t bit_number = 0; bit_number < MAX_LAYER_BITS; bit_number++) {
        source_layers_cache[storage_row][bit_number] ^= (-((layer & (1U << bit_number)) != 0) ^ source_layers_cache[storage_row][bit_number]) & (1U << storage_bit);
    }
}

/** \brief read source layers cache
 *
 * reads the cached keys stored when the layer was changed
 */
uint8_t read_source_layers_cache(keypos_t key) {
    const uint8_t key_number  = key.col + (key.row * MATRIX_COLS);
    const uint8_t storage_row = key_number / 8;
    const uint8_t storage_bit = key_number % 8;
    uint8_t       layer       = 0;

    for (uint8_t bit_number = 0; bit_number < MAX_LAYER_BITS; bit_number++) {
        layer |= ((source_layers_cache[storage_row][bit_number] & (1U << storage_bit)) != 0) << bit_number;
    }

    return layer;
}
#endif

action_t store_or_get_action(bool pressed, keypos_t key) {
#if !defined(NO_ACTION_LAYER) && !defined(STRICT_LAYER_RELEASE)
    if (disable_action_cache) {
        return layer_switch_get_action(key);
    }

    uint8_t layer;

    if (pressed) {
        layer = layer_switch_get_layer(key);
        update_source_layers_cache(key, layer);
    } else {
        layer = read_source_layers_cache(key);
    }
    return action_for_key(layer, key);
#else
    return layer_switch_get_action(key);
#endif
}

uint8_t layer_switch_get_layer(keypos_t key) {
#ifndef NO_ACTION_LAYER
    action_t action;
    action.code = ACTION_TRANSPARENT;

    uint32_t layers = layer_state | default_layer_state;
    /* check top layer first */
    for (int8_t i = KEYMAPS_COUNT - 1; i >= 0; i--) {
        if (layers & (1UL << i)) {
            action = action_for_key(i, key);
            if (action.code != ACTION_TRANSPARENT) {
                return i;
            }
        }
    }
    /* fall back to layer 0 */
    return 0;
#else
    return get_highest_layer(default_layer_state);
#endif
}



action_t layer_switch_get_action(keypos_t key)
{
    action_t action = { .code = ACTION_TRANSPARENT };

#ifndef NO_ACTION_LAYER
    uint32_t layers = layer_state | default_layer_state;
    /* check top layer first */
    for (int8_t i = 31; i >= 0; i--) {
        if (layers & (1UL<<i)) {
            action = action_for_key(i, key);
            if (action.code != ACTION_TRANSPARENT) {
                return action;
            }
        }
    }
    /* fall back to layer 0 */

    action = action_for_key(0, key);
    return action;
#else
    action = action_for_key(biton32(default_layer_state), key);
    return action;
#endif
}
