#include <stdint.h>
#include <gpio.h>
#include <i2c.h>
#include <printk.h>
#include <uart.h>
#include <unistd.h>
#include <lcd_driver.h>
#include <keypad_driver.h>
// #include <adc.h>
#include <systick.h>
#include <timer.h>

int kernel_main(void) {
    uint8_t row = 0;
    uint8_t col = 0;
    uint32_t last_print;

    uart_init(115200);
    systick_init(); /* Must precede LCD's systick_delay calls. */
    keypad_init();
    // adc_init();
    lcd_driver_init();

    last_print = systick_get_ticks();

    // blink LED every second
    timer_init(2, 16000, 1000);
    // gpio_init LED, actual blinking in tim2_c_handler

    while (1) {
        char c = keypad_read();
        if (c != '\0') { /* Do not display a null/no-key result. */
            switch (c) {
                case '*':
                    row = (uint8_t)(row == 0u ? 1u : 0u);
                    col = 0;
                    lcd_set_cursor(row, col);
                    break;
                case '#':
                    row = 0;
                    col = 0;
                    lcd_clear();
                    break;
                default: {
                    char input[2] = { c, '\0' };
                    lcd_print(input);
                    ++col;
                    if (col >= 16u) {
                        row = (uint8_t)(row == 0u ? 1u : 0u);
                        col = 0;
                        lcd_set_cursor(row, col);
                    }
                    break;
                }
            }
        }

        /* Non-blocking timing: keypad remains responsive between samples. */
        uint32_t now = systick_get_ticks();
        if ((uint32_t)(now - last_print) >= 1000u) {
            last_print = now;
            printk("Interrupt!\n");
        }
    }
}
