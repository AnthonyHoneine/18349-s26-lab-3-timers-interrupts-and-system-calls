/** @file kernel.c @brief Lab 3 hardware initialization and user launch. */
#include <stdint.h>
#include <gpio.h>
#include <uart.h>
#include <systick.h>
#include <keypad_driver.h>
#include <lcd_driver.h>
#include <timer.h>
#include <arm.h>
#include <kernel.h>

int kernel_main(void) {
    init_349();
    uart_init(115200);
    systick_init();
    keypad_init();
    lcd_driver_init();

    /* PWM0=PA1, PWM1=PB0: software-generated GPIO signals. */
    gpio_init(GPIO_A, 1, MODE_GP_OUTPUT, OUTPUT_PUSH_PULL,
              OUTPUT_SPEED_LOW, PUPD_NONE, 0);
    gpio_init(GPIO_B, 0, MODE_GP_OUTPUT, OUTPUT_PUSH_PULL,
              OUTPUT_SPEED_LOW, PUPD_NONE, 0);
    gpio_clr(GPIO_A, 1);
    gpio_clr(GPIO_B, 0);

    /* TIM2: 1MHz counter; ISR schedules each PWM edge dynamically. */
    timer_init(2, 16, 20000);
    enter_user_mode();
    for (;;) { /* enter_user_mode should not return. */ }
}
