/**
 * @file timer.c
 * @brief General-purpose timers and servo PWM control.
 */

#include <stdint.h>
#include <timer.h>
#include <rcc.h>
#include <nvic.h>
#include <gpio.h>

extern volatile uint8_t servo_active[2];
extern volatile uint8_t servo_angle[2];

struct tim2_5 {
  volatile uint32_t cr1; /**< 00 Control Register 1 */
  volatile uint32_t cr2; /**< 04 Control Register 2 */
  volatile uint32_t smcr; /**< 08 Slave Mode Control */
  volatile uint32_t dier; /**< 0C DMA/Interrupt Enable */
  volatile uint32_t sr; /**< 10 Status Register */
  volatile uint32_t egr; /**< 14 Event Generation */
  volatile uint32_t ccmr[2]; /**< 18-1C Capture/Compare Mode */
  volatile uint32_t ccer; /**< 20 Capture/Compare Enable */
  volatile uint32_t cnt; /**< 24 Counter Register */
  volatile uint32_t psc; /**< 28 Prescaler Register */
  volatile uint32_t arr; /**< 2C Auto-Reload Register */
  volatile uint32_t reserved_1; /**< 30 */
  volatile uint32_t ccr[4]; /**< 34-40 Capture/Compare */
  volatile uint32_t reserved_2; /**< 44 */
  volatile uint32_t dcr; /**< 48 DMA Control Register */
  volatile uint32_t dmar; /**< 4C DMA address for full transfer Register */
  volatile uint32_t or; /**< 50 Option Register */
};

struct tim2_5* const timer_base[] = {(void *)0x0,   // N/A - Don't fill out
                                     (void *)0x0,   // N/A - Don't fill out
                                     (void *)0x40000000,    // TODO: fill out address for TIMER 2
                                     (void *)0x40000400,    // TODO: fill out address for TIMER 3
                                     (void *)0x40000800,    // TODO: fill out address for TIMER 4
                                     (void *)0x40000C00};   // TODO: fill out address for TIMER 5

static const uint8_t timer_irq[] = {0, 0, 28, 29, 30, 50};

#define TIM_CEN (1 << 0)
#define TIM_UIE (1 << 0)
#define TIM_UIF (1 << 0)
#define TIM_UG  (1 << 0)
#define TIM_URS (1 << 2)

#define SERVO_FRAME_US 20000
#define SERVO_MIN_US 600
#define SERVO_DEGREE_US 10

void timer_init(int timer, uint32_t prescalar, uint32_t period) {
    if (timer < 2 || timer > 5 || prescalar == 0 ||
        prescalar > 65536 || period == 0) return;
    if ((timer == 3 || timer == 4) && period > 65536) return;

    struct rcc_reg_map *rcc = RCC_BASE;
    rcc->apb1_enr |= 1 << (timer - 2);
    (void)rcc->apb1_enr;

    struct tim2_5 *t = timer_base[timer];

    t->cr1 = TIM_URS;
    t->dier = 0;
    t->psc = prescalar - 1;
    t->arr = period - 1;
    t->cnt = 0;
    t->egr = TIM_UG;
    t->sr = 0;

    nvic_clear_pending(timer_irq[timer]);
    nvic_irq(timer_irq[timer], IRQ_ENABLE);

    t->dier = TIM_UIE;
    t->cr1 = TIM_CEN | TIM_URS;
}

void timer_disable(int timer) {
    if (timer < 2 || timer > 5) return;

    struct tim2_5 *t = timer_base[timer];
    nvic_irq(timer_irq[timer], IRQ_DISABLE);
    t->dier &= ~TIM_UIE;
    t->cr1 &= ~TIM_CEN;

    struct rcc_reg_map *rcc = RCC_BASE;
    rcc->apb1_enr &= ~(1 << (timer - 2));
}

void timer_clear_interrupt_bit(int timer) {
    if (timer < 2 || timer > 5) return;
    timer_base[timer]->sr = ~TIM_UIF;
}

/**
 * @brief Generates PWM pulses for two servos using TIM2.
 *
 * Each frame has up to three events: start both pulses,
 * lower the first channel, and lower the second channel.
 */
void tim2_c_handler(void) {
    static int phase = 0;
    static uint32_t first_width, second_width;
    static int first_channel, second_needed;

    struct tim2_5 *t = timer_base[2];
    uint32_t next_us;

    timer_clear_interrupt_bit(2);

    if (phase == 0) {
        uint32_t w0 = SERVO_MIN_US + servo_angle[0] * SERVO_DEGREE_US;
        uint32_t w1 = SERVO_MIN_US + servo_angle[1] * SERVO_DEGREE_US;

        int a0 = servo_active[0];
        int a1 = servo_active[1];

        if (a0) gpio_set(GPIO_A, 1);
        else gpio_clr(GPIO_A, 1);

        if (a1) gpio_set(GPIO_B, 0);
        else gpio_clr(GPIO_B, 0);

        if (!a0 && !a1) {
            next_us = SERVO_FRAME_US;
        } else {
            // Determine which servo pulse ends first.
            if (a0 && (!a1 || w0 <= w1)) {
                first_channel = 0;
                first_width = w0;
                second_width = w1;
                second_needed = a1 && w1 > w0;
            } else {
                first_channel = 1;
                first_width = w1;
                second_width = w0;
                second_needed = a0 && w0 > w1;
            }

            phase = 1;
            next_us = first_width;
        }

    } else if (phase == 1) {
        if (first_channel == 0) gpio_clr(GPIO_A, 1);
        else gpio_clr(GPIO_B, 0);

        if (second_needed) {
            phase = 2;
            next_us = second_width - first_width;
        } else {
            // Both channels may have the same pulse width.
            if (servo_active[0] && servo_active[1]) {
                if (first_channel == 0) gpio_clr(GPIO_B, 0);
                else gpio_clr(GPIO_A, 1);
            }

            phase = 0;
            next_us = SERVO_FRAME_US - first_width;
        }

    } else {
        if (first_channel == 0) gpio_clr(GPIO_B, 0);
        else gpio_clr(GPIO_A, 1);

        phase = 0;
        next_us = SERVO_FRAME_US - second_width;
    }

    // Schedule the next PWM transition.
    t->cr1 &= ~TIM_CEN;
    t->arr = next_us - 1;
    t->cnt = 0;
    t->egr = TIM_UG;
    t->cr1 |= TIM_CEN;
}

void tim3_c_handler(void) {
    timer_clear_interrupt_bit(3);
}

void tim4_c_handler(void) {
    timer_clear_interrupt_bit(4);
}

void tim5_c_handler(void) {
    timer_clear_interrupt_bit(5);
}
