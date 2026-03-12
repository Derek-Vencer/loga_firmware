
#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>
#include <stdbool.h>

#define rappySnappy // lyon (rappy snappy when under 8k report rate will easily let the thread frozen
#define rappyTrigger
#define eepromClear
#define socd
#define buttonLock
#define modTap
// #define test8_thread // seperate send keycode to tmr in another thread
#define toggleKey
// #define socd_new

#ifdef toggleKey
void    toggle_key_set(uint8_t layer, uint8_t row, uint8_t col, uint8_t keycode);
uint8_t toggle_key_get(uint8_t layer, uint8_t row, uint8_t col);
#endif // toggleKey


#if (MATRIX_COLS <= 8)
typedef  uint8_t    matrix_row_t;
#elif (MATRIX_COLS <= 16)
typedef  uint16_t   matrix_row_t;
#elif (MATRIX_COLS <= 32)
typedef  uint32_t   matrix_row_t;
#else
#error "MATRIX_COLS: invalid value"
#endif

#define MATRIX_IS_ON(row, col)  (matrix_get_row(row) && (1<<col))


#ifdef __cplusplus
extern "C" {
#endif

/* number of matrix rows */
uint8_t matrix_rows(void);
/* number of matrix columns */
uint8_t matrix_cols(void);
/* should be called at early stage of startup before matrix_init.(optional) */
void matrix_setup(void);
/* intialize matrix for scaning. */
void matrix_init(void);
/* scan all key states on matrix */
uint8_t matrix_scan(void);
/* whether modified from previous scan. used after matrix_scan. */
bool matrix_is_modified(void) __attribute__ ((deprecated));
/* whether a swtich is on */
bool matrix_is_on(uint8_t row, uint8_t col);
/* matrix state on row */
matrix_row_t matrix_get_row(uint8_t row);
/* print matrix for debug */
void matrix_print(void);


/* power control */
void matrix_power_up(void);
void matrix_power_down(void);


typedef struct {
	//uint8_t layer;
    bool enable;
	uint8_t row;
	uint8_t col;
    uint8_t counts;
    uint16_t keycode[RAPID_TRIGGER_LEVELS];
} rapidTrigger_config_t;

typedef struct {
	rapidTrigger_config_t key;
    uint16_t onLevel[RAPID_TRIGGER_LEVELS];
    uint16_t offLevel[RAPID_TRIGGER_LEVELS];
    int8_t lastStep;
    int16_t triggeredLevel;
    uint32_t debounceTimer;   
} rapidTriggerKey_config_t;

// typedef struct {
// 	//uint8_t layer;
//     bool enable;
// 	uint8_t row;
// 	uint8_t col;
//     uint8_t counts;
//     uint16_t onLevel[RAPID_TRIGGER_LEVELS];
//     uint16_t offLevel[RAPID_TRIGGER_LEVELS];
//     uint16_t keycode[RAPID_TRIGGER_LEVELS];
//     int8_t lastStep;
//     int16_t triggeredLevel;
//     uint32_t debounceTimer;   
// } rapidTrigger_config_t;

#ifdef modTap

#define MT_CAP_MAP 32          // 依你的實際大小

typedef struct {
    uint8_t L, R, C;           // 建議之後可改成 layer,row,col 比較清楚
    uint8_t hold, click;
    bool    used;
} mt_entry_t;

extern mt_entry_t mt_map[MT_CAP_MAP];   // 只有宣告，沒有初始化
#endif //modTap

// move from matrix.c because i want to use these in via.c (lyon)
#define TriggerMax	1500
#define TriggerMin  3
#define TriggerInitial DEFAULT_TRIGGER_LEVEL

#define ReleaseMax	100
#define ReleaseMin	DEFAULT_RELEASE_LEVEL
#define ReleaseInitial 5

#ifdef __cplusplus
}
#endif

#endif
