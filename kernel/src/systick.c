/**
 * @file systick.c
 * @brief SysTick initialization, millisecond delays, and uptime counter.
 */

#include <stdint.h>
#include <systick.h>

/* Cortex-M4 SysTick registers (PM0214). */
#define STK_CTRL (*(volatile uint32_t *)0xE000E010u)
#define STK_LOAD (*(volatile uint32_t *)0xE000E014u)
#define STK_VAL  (*(volatile uint32_t *)0xE000E018u)

#define STK_CTRL_ENABLE    (1u << 0)
#define STK_CTRL_TICKINT   (1u << 1)
#define STK_CTRL_CLKSOURCE (1u << 2)

#define SYSTEM_CLOCK_HZ 16000000u
#define SYSTICK_HZ       1000u

static volatile uint32_t elapsed_ticks = 0u;

void systick_init() {
    STK_CTRL = 0u; // Disable SysTick while configuring.
    elapsed_ticks = 0u;
    STK_LOAD = (SYSTEM_CLOCK_HZ / SYSTICK_HZ) - 1u;
    STK_VAL = 0u;  // Clear current value and COUNTFLAG.
    STK_CTRL = STK_CTRL_CLKSOURCE | STK_CTRL_TICKINT | STK_CTRL_ENABLE;
}

void systick_delay(uint32_t ticks) {
    uint32_t start = systick_get_ticks();
    while ((uint32_t)(systick_get_ticks() - start) < ticks) {
        // Busy wait for the requested number of milliseconds.
    }
}

uint32_t systick_get_ticks() {
    return elapsed_ticks;
}

void systick_c_handler() {
    ++elapsed_ticks;
}
