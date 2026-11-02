/** @file uart_polling.c @brief Standalone polling USART2 API (Lab 2 compatibility). */
#include <stdint.h>
#include <gpio.h>
#include <rcc.h>
#include <uart_polling.h>

struct uart_polling_regs {
    volatile uint32_t sr, dr, brr, cr1, cr2, cr3, gtpr;
};
#define UART2 ((struct uart_polling_regs *)0x40004400u)
#define UART_CLK 16000000u
#define UART_UE (1u << 13)
#define UART_TE (1u << 3)
#define UART_RE (1u << 2)
#define UART_TXE (1u << 7)
#define UART_RXNE (1u << 5)

void uart_polling_init(int baud) {
    if (baud <= 0) return;
    struct rcc_reg_map *rcc = RCC_BASE;
    rcc->apb1_enr |= (1u << 17);
    (void)rcc->apb1_enr;
    gpio_init(GPIO_A, 2, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT7);
    gpio_init(GPIO_A, 3, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT7);
    UART2->cr1 = 0u;
    UART2->brr = (UART_CLK + (uint32_t)baud / 2u) / (uint32_t)baud;
    UART2->cr2 = 0u;
    UART2->cr3 = 0u;
    UART2->cr1 = UART_UE | UART_TE | UART_RE;
}
void uart_polling_put_byte(char c) {
    while (!(UART2->sr & UART_TXE)) { }
    UART2->dr = (uint8_t)c;
}
char uart_polling_get_byte(void) {
    while (!(UART2->sr & UART_RXNE)) { }
    return (char)UART2->dr;
}
