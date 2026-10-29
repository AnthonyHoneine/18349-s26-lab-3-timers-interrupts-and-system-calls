/**
 * @file
 *
 * @brief
 *
 * @date
 *
 * @author
 */

#include <unistd.h>
#include <timer.h>
#include <rcc.h>
#include <nvic.h>
#include <gpio.h>

extern volatile uint8_t servo_active[2];
extern volatile uint8_t servo_angle[2];

#define UNUSED __attribute__((unused))

/** @brief tim2_5 */
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


void timer_init(int timer, uint32_t prescalar, uint32_t period) {
  struct rcc_reg_map *rcc = (struct rcc_reg_map *)RCC_BASE;
  rcc->apb1_enr |= (1 << (timer-2)); // bits 0-3 for tim2-5

  struct tim2_5 *tim = timer_base[timer];
  tim->psc = prescalar - 1; // ck_cnt=f_ck/(psc[15:0] + 1)
  tim->arr = period;
  // interrupt event is automatically generated when this is enabled
  tim->dier |= 1; // update interrupt enable
  tim->cr1 |= 1; // enable counter

  int irq_num = 0;
  switch (timer) {
    case 2: 
      irq_num = 28;
      break;
    case 3: 
      irq_num = 29;
      break;
    case 4: 
      irq_num = 30;
      break;
    case 5: 
      irq_num = 50;
      break;
  }

  if (irq_num > 0) {
    nvic_irq(irq_num, IRQ_ENABLE);
  }
}

void timer_disable(int timer) {
  struct rcc_reg_map *rcc = (struct rcc_reg_map *)RCC_BASE;
  rcc->apb1_enr &= ~(1 << (timer-2));

  struct tim2_5 *t = timer_base[timer];
  t->cr1 &= ~1;
}

void timer_clear_interrupt_bit(int timer) {
  struct tim2_5 *t = timer_base[timer];
  t->sr &= ~1; // clear interrupt flag (dk if read only)
}

void tim2_c_handler(void) {
  // blink led gpio
  // gpio_set(GPIO_A, 0);
  timer_clear_interrupt_bit(2);

  static uint32_t tick_counter = 0;

  if (servo_active[0]) {
    if (tick_counter == 0) {
      gpio_set(GPIO_A, 1);
    } else if (tick_counter == (uint32_t)(60 + servo_angle[0])) {
      gpio_clr(GPIO_A, 1);
    }
  }
  if (servo_active[1]) {
    if (tick_counter == 0) {
      gpio_set(GPIO_B, 0);
    } else if (tick_counter == (uint32_t)(60 + servo_angle[1])) {
      gpio_clr(GPIO_B, 0);
    }
  }

  tick_counter++;
  if (tick_counter >= 2000) tick_counter = 0;
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