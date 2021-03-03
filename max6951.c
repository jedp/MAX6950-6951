#include "max6951.h"

/*
 * Write sequence:
 *
 * 1. Take CLK low.
 * 2. Take ~CS low.
 * 3. Clock 16 bits of data, D15 to D0, in that order.
 * 4. Take ~CS high.
 *
 * CLK and DIN are only observed when ~CS is low, so these pins
 * can be used to send data to other devices.
 *
 * Digit registers are implemented by two planes of 8-byte SRAM, P0 and P1.
 *
 *
 */

/**
 * MAX6950/MAX6951 Datasheet, Table 2, "Register Address Map".
 */
typedef max695x_cmd_addr_t uint16_t;
enum max695x_cmd_addr {
  MAX695X_REG_NO_OP             = 0x00 << 8,
  MAX695X_REG_DECODE_MODE       = 0x01 << 8,
  MAX695X_REG_INTENSITY         = 0x02 << 8,
  MAX695X_REG_SCAN_LIMIT        = 0x03 << 8,
  MAX695X_REG_CONFIGURE         = 0x04 << 8,
  // There is no 0x05.
  // 0x06 is reserved.
  MAX695X_REG_TEST              = 0x07 << 8,
  MAX695X_DIG0_PLANE_P0_ONLY    = 0x20 << 8,
  MAX695X_DIG1_PLANE_P0_ONLY    = 0x21 << 8,
  MAX695X_DIG2_PLANE_P0_ONLY    = 0x22 << 8,
  MAX695X_DIG3_PLANE_P0_ONLY    = 0x23 << 8,
  MAX695X_DIG4_PLANE_P0_ONLY    = 0x24 << 8,
  MAX695X_DIG5_PLANE_P0_ONLY    = 0x25 << 8,
  MAX695X_DIG6_PLANE_P0_ONLY    = 0x26 << 8,
  MAX695X_DIG7_PLANE_P0_ONLY    = 0x27 << 8,
  MAX695X_DIG0_PLANE_P1_ONLY    = 0x40 << 8,
  MAX695X_DIG1_PLANE_P1_ONLY    = 0x41 << 8,
  MAX695X_DIG2_PLANE_P1_ONLY    = 0x42 << 8,
  MAX695X_DIG3_PLANE_P1_ONLY    = 0x43 << 8,
  MAX695X_DIG4_PLANE_P1_ONLY    = 0x44 << 8,
  MAX695X_DIG5_PLANE_P1_ONLY    = 0x45 << 8,
  MAX695X_DIG6_PLANE_P1_ONLY    = 0x46 << 8,
  MAX695X_DIG7_PLANE_P1_ONLY    = 0x47 << 8,
  MAX695X_DIG0_PLANES_P0_AND_P1 = 0x60 << 8,
  MAX695X_DIG1_PLANES_P0_AND_P1 = 0x61 << 8,
  MAX695X_DIG2_PLANES_P0_AND_P1 = 0x62 << 8,
  MAX695X_DIG3_PLANES_P0_AND_P1 = 0x63 << 8,
  MAX695X_DIG4_PLANES_P0_AND_P1 = 0x64 << 8,
  MAX695X_DIG5_PLANES_P0_AND_P1 = 0x65 << 8,
  MAX695X_DIG6_PLANES_P0_AND_P1 = 0x66 << 8,
  MAX695X_DIG7_PLANES_P0_AND_P1 = 0x67 << 8,
}

/* Configuration register. */
#define D_OPERATION_MODE 0
#define D_CONFIG_UPDATED 1
#define D_BLINK_RATE 2
#define D_BLINK_ENABLE 3
#define D_BLINK_TIMING_RESET 4
#define D_RESET_DATA 5

/*
 * MAX7219 Datasheet, Table 5, "Code B Font".
 */
enum {
  MAX695X_B_CODE_0 = 0x0,
  MAX695X_B_CODE_1,
  MAX695X_B_CODE_2,
  MAX695X_B_CODE_3,
  MAX695X_B_CODE_4,
  MAX695X_B_CODE_5,
  MAX695X_B_CODE_6,
  MAX695X_B_CODE_7,
  MAX695X_B_CODE_8,
  MAX695X_B_CODE_9,
  MAX695X_B_CODE_HYPHEN,
  MAX695X_B_CODE_E,
  MAX695X_B_CODE_H,
  MAX695X_B_CODE_L,
  MAX695X_B_CODE_P,
  MAX695X_B_CODE_BLANK
};

/*
 * Exponentiation by squaring.
 */
int ipow10(int exp) {
  if (exp < 1) return 1;

  int base = 10;
  int result = 1;

  while (1) {
    if (exp & 1) result *= base;

    exp >>= 1;

    if (!exp) break;

    base *= base;
  }

  return result;
}

max695x_err_t _write(max695x_t *max695x, uint8_t cmd, uint8_t data) {
  uint8_t message[2];
  message[0] = cmd;
  message[1] = data;

  HAL_GPIO_WritePin(max695x->port_cs, max695x->pin_cs, GPIO_PIN_RESET);
  HAL_SPI_Transmit(max695x->hspi, (uint8_t *) message, 2, TIMEOUT_MS);
  HAL_GPIO_WritePin(max695x->port_cs, max695x->pin_cs, GPIO_PIN_SET);

  // TODO return errors
  return MAX695X_ERR_NO_ERROR;
}

/*
 * Configuration Register.
 *
 * Bit position D1 should always be written with a 0 when the config
 * register is updated.
 *
 * ADDR   D7  D6  D5  D4  D3  D2  D1  D0
 * 0x04    x   x   R   T   E   B   0   S
 *
 * S: 0 shutdown, 1 normal operation
 * B: 0 slow, 1 fast blink
 * E: 0 disable,  1 enable blink
 * T: 1 reset blink timing
 * R: 1 clear all data in planes P0 and P1
 */
max695x_err_t max695x_set_blink_mode(max695x_t *max695x, max695x_blink_mode_t blink_mode) {
  // Always set D1 to 0 when updating config.
  uint8_t data = 0;

  if (blink_mode != MAX6951_BLINK_DISABLED) {
    // Set blink enable 1.
    data |= 1 << D_BLINK_ENABLE;

    if (blink_mode == MAX6951_BLINK_FAST) {
      // Set blink rate to 1.
      data |= 1 << D_BLINK_RATE;
    }
  }

  return _write(max695x, MAX695X_REG_CONFIGURE, data);
}

max695x_err_t max695x_clear_digit_data(max695x_t *max695x) {
  uint8_t data = 1 << D_RESET_DATA;
  return _write(MAX695X_REG_CONFIGURE, data);
}

max695x_err_t max695x_display_mode(max695x_t *max695x, max695x_display_mode_t display_mode) {
  uint8_t data = display_mode == MAX695X_MODE_NORMAL ? (uint8_t) 1 : (uint8_t) 0;

  return _write(max695x, MAX695X_REG_TEST, data);
}

max695x_err_t max695x_set_num_digits(max695x_t *max695x, uint32_t digits) {
  if (digits < 1 || digits > 8) return MAX695X_ERR_INVALID_ARGUMENT;

  // 1 digit = 0x00, 2 digits = 0x01, ..., 7 digits = 0x07.
  uint8_t data = ((uint8_t) digits - 1) & 0x07;

  return _write(MAX695X_REG_SCAN_LIMIT, data);
}

max695x_err_t max695x_set_intensity(max695x_t *max695x, uint32_t intensity) {
  if (intensity > 15) return MAX695X_ERR_INVALID_ARGUMENT;

  uint8_t data = (uint8_t intensity) & 0x0F;
  return _write(MAX594X_REG_INTENSITY, data);
}

max695x_err_t max695x_set_decode_mode(max695x_t *max695x, uint32_t digits_mask) {
  if (digits_mask > 0x07) return MAX695X_ERR_INVALID_ARGUMENT;

  uint8_t data = (uint8_t digits_mask) & 0x07;
  return _write(MAX594X_REG_DECODE_MODE, data);
}

max695x_err_t max695x_display_digit(max695x_t *max695x,
                                    max695x_digit_plane_t digit_plane,
                                    uint32_t digit,
                                    uint32_t value) {
  if (digit_plane != MAX695X_PLANE_P0_ONLY &&
      digit_plane != MAX695X_PLANE_P1_ONLY &&
      digit_plane != MAX695X_PLANES_P0_AND_P1) return MAX695X_ERR_INVALID_ARGUMENT;

  if (digit > 7) return MAX695X_ERR_INVALID_ARGUMENT;

  // It's up to the caller to know whether they are using the hex decoder or not.
  if (value > 15) return MAX695X_ERR_INVALID_ARGUMENT;

  uint8_t cmd = (uint8_t) digit_plane + (uint8_t) value;
  uint8_t data = (uint8_t) value & 0xF;

  return _write(max695x, cmd, data);
}

