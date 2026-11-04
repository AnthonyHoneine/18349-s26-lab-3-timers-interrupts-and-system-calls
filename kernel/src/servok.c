/** @file servok.c @brief Kernel-side configuration for two SG90 servos. */
#include <stdint.h>
#include <gpio.h>
#include <syscall.h>

volatile uint8_t servo_active[2] = {0u, 0u};
volatile uint8_t servo_angle[2] = {90u, 90u};

int sys_servo_enable(uint8_t channel, uint8_t enabled) {
    if (channel >= 2u || enabled > 1u) return -1;
    /* The timer ISR can only preempt SVC after it returns, provided priorities
       are configured normally; the active flag is one atomic byte store. */
    if (!enabled) {
        servo_active[channel] = 0u;
        if (channel == 0u)
            gpio_clr(GPIO_A, 1);
        else
            gpio_clr(GPIO_B, 0);
    } else {
        servo_active[channel] = 1u;
    }
    return 0;
}

int sys_servo_set(uint8_t channel, uint8_t angle) {
    if (channel >= 2u || angle > 180u) return -1;
    servo_angle[channel] = angle;
    return 0;
}
