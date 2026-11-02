/** @file timer.c @brief TIM2-TIM5 general-purpose timers and servo ISR. */
#include <stdint.h>
#include <timer.h>
#include <rcc.h>
#include <nvic.h>
#include <gpio.h>

extern volatile uint8_t servo_active[2];
extern volatile uint8_t servo_angle[2];

struct tim2_5 {
    volatile uint32_t cr1, cr2, smcr, dier, sr, egr;
    volatile uint32_t ccmr[2], ccer, cnt, psc, arr, reserved_1;
    volatile uint32_t ccr[4], reserved_2, dcr, dmar, or;
};

static struct tim2_5 *const timer_base[] = {
    (void *)0, (void *)0, (void *)0x40000000u,
    (void *)0x40000400u, (void *)0x40000800u, (void *)0x40000C00u
};
static const uint8_t timer_irq[] = {0u, 0u, 28u, 29u, 30u, 50u};
#define TIM_CEN  (1u << 0)
#define TIM_UIE  (1u << 0)
#define TIM_UIF  (1u << 0)
#define TIM_UG   (1u << 0)
#define SERVO_FRAME_US 20000u
#define SERVO_MIN_US 600u

void timer_init(int timer, uint32_t prescalar, uint32_t period) {
    if (timer < 2 || timer > 5 || prescalar == 0u ||
        prescalar > 65536u || period == 0u) return;
    if ((timer == 3 || timer == 4) && period > 65536u) return;
    struct rcc_reg_map *rcc = RCC_BASE;
    rcc->apb1_enr |= (1u << (timer - 2));
    (void)rcc->apb1_enr;
    struct tim2_5 *t = timer_base[timer];
    t->cr1 = 0u;
    t->dier = 0u;
    t->psc = prescalar - 1u;
    t->arr = period - 1u;
    t->cnt = 0u;
    t->egr = TIM_UG; /* Update loads PSC immediately. */
    t->sr = 0u;      /* Clear UIF produced by UG. */
    nvic_clear_pending(timer_irq[timer]);
    nvic_irq(timer_irq[timer], IRQ_ENABLE);
    t->dier = TIM_UIE;
    t->cr1 = TIM_CEN;
}

void timer_disable(int timer) {
    if (timer < 2 || timer > 5) return;
    struct tim2_5 *t = timer_base[timer];
    t->dier &= ~TIM_UIE;
    t->cr1 &= ~TIM_CEN;
    nvic_irq(timer_irq[timer], IRQ_DISABLE);
    struct rcc_reg_map *rcc = RCC_BASE;
    rcc->apb1_enr &= ~(1u << (timer - 2));
}

void timer_clear_interrupt_bit(int timer) {
    if (timer < 2 || timer > 5) return;
    /* TIM status bits are rc_w0: write 0 to UIF, 1 to others. */
    timer_base[timer]->sr = ~TIM_UIF;
}

/* Event-driven software PWM: at most 3 TIM2 updates per 20ms frame.
 * At 1MHz, 1 timer count is 1us. No high-rate busy ISR is necessary. */
void tim2_c_handler(void) {
    static uint8_t phase = 0u; /* 0=start frame, 1=first fall, 2=second fall */
    static uint32_t first_width, second_width;
    static uint8_t first_ch, second_ch, second_needed;
    struct tim2_5 *t = timer_base[2];
    uint32_t next_us;
    timer_clear_interrupt_bit(2);
    if (phase == 0u) {
        uint32_t w0 = SERVO_MIN_US + (uint32_t)servo_angle[0] * 10u;
        uint32_t w1 = SERVO_MIN_US + (uint32_t)servo_angle[1] * 10u;
        uint8_t a0 = servo_active[0];
        uint8_t a1 = servo_active[1];
        if (a0) gpio_set(GPIO_A, 1); else gpio_clr(GPIO_A, 1);
        if (a1) gpio_set(GPIO_B, 0); else gpio_clr(GPIO_B, 0);
        if (!a0 && !a1) {
            next_us = SERVO_FRAME_US;
        } else {
            /* Sort active falling edges. */
            if (a0 && (!a1 || w0 <= w1)) {
                first_ch = 0u; first_width = w0;
                second_ch = 1u; second_width = w1;
                second_needed = a1 && w1 > w0;
            } else {
                first_ch = 1u; first_width = w1;
                second_ch = 0u; second_width = w0;
                second_needed = a0 && w0 > w1;
            }
            phase = 1u;
            next_us = first_width;
        }
    } else if (phase == 1u) {
        if (first_ch == 0u) gpio_clr(GPIO_A, 1);
        else gpio_clr(GPIO_B, 0);
        if (second_needed) {
            phase = 2u;
            next_us = second_width - first_width;
        } else {
            /* Also clear the channel with an identical pulse width. */
            if (servo_active[0] && servo_active[1] && !second_needed) {
                if (first_ch == 0u) gpio_clr(GPIO_B, 0);
                else gpio_clr(GPIO_A, 1);
            }
            phase = 0u;
            next_us = SERVO_FRAME_US - first_width;
        }
    } else {
        if (second_ch == 0u) gpio_clr(GPIO_A, 1);
        else gpio_clr(GPIO_B, 0);
        phase = 0u;
        next_us = SERVO_FRAME_US - second_width;
    }
    /* Program next interrupt, taking immediate PSC/ARR effect via UG.
     * UG sets UIF, which must be cleared before returning. */
    t->cr1 &= ~TIM_CEN;
    t->arr = next_us - 1u;
    t->cnt = 0u;
    t->egr = TIM_UG;
    t->sr = 0u;
    t->cr1 |= TIM_CEN;
}
void tim3_c_handler(void) { timer_clear_interrupt_bit(3); }
void tim4_c_handler(void) { timer_clear_interrupt_bit(4); }
void tim5_c_handler(void) { timer_clear_interrupt_bit(5); }
