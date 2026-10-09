/**
 * @file lcd_driver.c
 * LCD driver over the I2C-to-4-bit interface.
 */
#include <stdint.h>
#include <unistd.h>
#include <i2c.h>
#include <lcd_driver.h>
#include <systick.h>

#define LCD_ADDR    (0x4E >> 1)
#define INSTRUCTION 0u
#define DATA        1u
#define LCD_CLEAR   0x01u
#define LCD_ENTRY   0x06u
#define LCD_DISPLAY_ON 0x0Cu
#define LCD_FUNCTION_SET 0x28u /* 4-bit, 2 lines, 5x8 font */

/* The I2C backpack uses E=bit 2, backlight=bit 3, RS=bit 0. */
static void lcd_send_nibble(uint8_t nibble, uint8_t rs) {
    uint8_t high = (uint8_t)((nibble & 0xF0u) | 0x0Cu | (rs & 1u));
    uint8_t buf[2] = {high, (uint8_t)(high & (uint8_t)~0x04u)};
    i2c_master_write(buf, 2, LCD_ADDR);
}

void lcd_send_instruction(uint8_t byte, uint8_t rs) {
    lcd_send_nibble(byte & 0xF0u, rs);
    lcd_send_nibble((uint8_t)(byte << 4), rs);
    /* HD44780 typical instructions need <= 37 us: round up to 1 ms. */
    if (rs == INSTRUCTION && byte == LCD_CLEAR) {
        systick_delay(2000); /* Lab 3 Section 6 explicitly requires 2 seconds. */
    } else {
        systick_delay(1);
    }
}

void lcd_driver_init(void) {
    i2c_master_init(0);
    systick_delay(40);  /* power-on wait (> 15 ms) */

    /* The HD44780 is not yet in 4-bit mode: send single nibbles. */
    lcd_send_nibble(0x30u, INSTRUCTION);
    systick_delay(5);   /* > 4.1 ms */
    lcd_send_nibble(0x30u, INSTRUCTION);
    systick_delay(1);   /* > 100 us */
    lcd_send_nibble(0x30u, INSTRUCTION);
    systick_delay(1);
    lcd_send_nibble(0x20u, INSTRUCTION); /* enter 4-bit mode */
    systick_delay(1);

    lcd_send_instruction(LCD_FUNCTION_SET, INSTRUCTION);
    lcd_send_instruction(0x08, INSTRUCTION); // Display off
    lcd_clear();
    lcd_send_instruction(LCD_ENTRY, INSTRUCTION);
    lcd_send_instruction(LCD_DISPLAY_ON, INSTRUCTION);
}

void lcd_print(char *input) {
    if (input == 0) return;
    for (int i = 0; input[i] != '\0'; ++i) {
        /* Write ASCII directly (not only numeric characters). */
        lcd_send_instruction((uint8_t)input[i], DATA);
    }
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    if (row > 1u || col >= 16u) return;
    lcd_send_instruction((uint8_t)(0x80u | (row ? 0x40u : 0x00u) | col), INSTRUCTION);
}

void lcd_clear(void) {
    lcd_send_instruction(LCD_CLEAR, INSTRUCTION);
}
