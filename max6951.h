/*
 * Library to program the MAX6950/6951 LED driver via SPI using STM32 HAL.
 *
 * By Jed Parsons
 *
 * Pinout
 *
 * CLK: Serial clock input.
 * ~CS: Chip select; set low to clock data in.
 * DIN: Data input; sampled on rising edge of clock.
 */

#ifndef __MAX6951_H
#define __MAX6951_H

#ifdef __cplusplus
extern "C" {
#endif

// To display the decimal point, AND it with the digit value.
#define MAX695X_DP 0x80

/*
 * No-decode data bits and corresponding segment lines.
 * Datasheet Table 16.
 *
 *        7  6  5  4  3  2  1  0
 * Line  DP  a  b  c  d  e  f  g
 *
 * To display the DP in either of the two decode modes, AND the digit
 * with 0x80.
 */
#define SSEG_A   0b01111101
#define SSEG_B   0b00011111
#define SSEG_C   0b00001101
#define SSEG_D   0b00111101
#define SSEG_E   0b01001111
#define SSEG_F   0b01000111
#define SSEG_G   0b01011110
#define SSEG_H   0b00010111
#define SSEG_I   0b01000100
#define SSEG_J   0b01011000
#define SSEG_K   0b01010111
#define SSEG_L   0b00001110
#define SSEG_M   0b01010101
#define SSEG_N   0b00010101
#define SSEG_O   0b00011101
#define SSEG_P   0b01100111
#define SSEG_Q   0b01110011
#define SSEG_R   0b00000101
#define SSEG_S   0b01011010
#define SSEG_T   0b00001111
#define SSEG_U   0b00011100
#define SSEG_V   0b00101010
#define SSEG_W   0b00101011
#define SSEG_X   0b00010100
#define SSEG_Y   0b00111011
#define SSEG_Z   0b01101100
#define SSEG_0   0b01111110
#define SSEG_1   0b00110000
#define SSEG_2   0b01101101
#define SSEG_3   0b01111001
#define SSEG_4   0b00110011
#define SSEG_5   0b01011011
#define SSEG_6   0b01011111
#define SSEG_7   0b01110000
#define SSEG_8   0b01111111
#define SSEG_9   0b01111011
#define SSEG_DP  0b10000000
#define SSEG_NEG 0b00000001
 
#include <stdint.h>
#include "stm32fxxx_hal.h" // Change to whatever you're using.

typedef enum {
  MAX695X_ERR_NO_ERROR = 0,
  MAX695X_ERR_INVALID_ARGUMENT = 1,
} max695x_err_t;

typedef uint8_t max695x_display_mode_t;
enum max695x_display_mode {
  MAX695X_MODE_TEST = 0,
  MAX695X_MODE_NORMAL = 1,
};

typedef uint8_t max695x_blink_mode_t;
enum max695x_blink_mode {
  MAX695X_BLINK_SLOW = 0,
  MAX695X_BLINK_FAST = 1,
  MAX695X_BLINK_DISABLED = 0xFF;
};

typedef uint8_t max695x_digit_plane_t;
enum max695x_digit_plane {
  MAX695X_PLANE_P0_ONLY = 0x20,
  MAX695X_PLANE_P1_ONLY = 0x40,
  MAX695X_PLANES_P0_AND_P1 = 0x60,
};

typedef struct {
  SPI_HandleTypeDef *hspi;
  GPIO_TypeDef * port_cs;
  uint16_t pin_cs;
} max695x_t;


max695x_err_t max695x_set_blink_mode(max695x_t *max695x, max695x_blink_mode_t blink_mode);

max695x_err_t max695x_clear_digit_data(max695x_t *max695x);

max695x_err_t max695x_display_mode(max695x_t *max695x, max695x_display_mode_t display_mode);

/*
 * Sets the number of digits to display, from 1 to 8.  This has the side-effect
 * of altering the brightness of the digits. According to the datasheet, if you
 * are using a 6950 next to a 6951, you can make their brightnesses match by
 * setting the 6951 to display the same number of digits, even though it's
 * physically only capable of displaying 5.
 */
max695x_err_t max695x_set_num_digits(max695x_t *max695x, uint32_t digits);

/* 
 * There are 16 intensity steps. Valid values: 0x0 to 0xF.
 *
 * Typical segment current mA is 2.5mA per step.  Datasheet Table 13:
 * 0x0   2.5 mA
 * 0x1   5
 * 0x2   7.5
 * 0x3  10
 * 0x4  12.5
 * 0x5  15
 * 0x6  17.5
 * 0x7  20
 * 0x8  22.5
 * 0x9  25
 * 0xA  27.5
 * 0xB  30
 * 0xC  32.5
 * 0xD  35
 * 0xE  37.5
 * 0xF  37.5     (0xE and 0xF are the same)
 */
max695x_err_t max695x_set_intensity(max695x_t *max695x, uint32_t intensity);

/*
 * Use hex decoding for the eight digits as specified in the mask.
 *
 * For example, to use hex decoding for the first, second, and fifth digits:
 * digits_mask = (1 << 0) | (1 << 1) | (1 << 4)
 *
 */
max695x_err_t max695x_set_decode_mode(max695x_t *max695x, uint32_t digits_mask);

/* 
 * Digit must be between 0 and 7 inclusive.
 *
 * Be sure you set a value that's consistent with the decode mode you have set.
 * To display the decimal point, AND the digit with MAX695X_DP.
 */
max695x_err_t max695x_display_digit(max695x_t *max695x,
                                    max695x_digit_plane_t digit_plane,
                                    uint32_t digit,
                                    uint32_t value);

max695x_err_t max695x_display_number(max695x_t *max695x,
                                     max695x_digit_plane_t digit_plane,
                                     uint32_t value);

#ifdef __cplusplus
}
#endif

#endif  // __MAX6951_H

