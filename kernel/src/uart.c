/**
 * @file uart.c
 * @brief Interrupt-driven USART2 with independent TX and RX ring buffers.
 */
#include <stdint.h>
#include <gpio.h>
#include <rcc.h>
#include <uart.h>
#include <nvic.h>

#define USART2_IRQ 38u
#define UART2_BASE ((struct uart_reg_map *)0x40004400u)
#define RCC_USART2EN (1u << 17)
#define USART_UE    (1u << 13)
#define USART_TE    (1u << 3)
#define USART_RE    (1u << 2)
#define USART_RXNEIE (1u << 5)
#define USART_TXEIE  (1u << 7)
#define USART_RXNE   (1u << 5)
#define USART_TXE    (1u << 7)
#define USART_TC     (1u << 6)
#define UART_CLOCK_HZ 16000000u

/* Change this constant to test different sizes, including 1. */
#ifndef UART_BUFFER_SIZE
#define UART_BUFFER_SIZE 128u
#endif
#if UART_BUFFER_SIZE < 1
#error UART_BUFFER_SIZE must be at least 1
#endif

struct uart_reg_map {
    volatile uint32_t SR, DR, BRR, CR1, CR2, CR3, GTPR;
};

/* Count distinguishes full from empty even with a one-byte buffer. */
struct ring {
    uint8_t bytes[UART_BUFFER_SIZE];
    uint32_t head, tail, count;
};
static struct ring tx, rx;

static uint32_t next_index(uint32_t n) {
    ++n;
    return (n == UART_BUFFER_SIZE) ? 0u : n;
}
static int push(struct ring *q, uint8_t c) {
    if (q->count == UART_BUFFER_SIZE) return -1;
    q->bytes[q->tail] = c;
    q->tail = next_index(q->tail);
    ++q->count;
    return 0;
}
static int pop(struct ring *q, uint8_t *c) {
    if (q->count == 0u) return -1;
    *c = q->bytes[q->head];
    q->head = next_index(q->head);
    --q->count;
    return 0;
}

/* Preserve the caller's PRIMASK, including a preexisting interrupt mask. */
static uint32_t irq_save(void) {
    uint32_t mask;
    __asm volatile ("mrs %0, primask\n cpsid i" : "=r" (mask) :: "memory");
    return mask;
}
static void irq_restore(uint32_t mask) {
    __asm volatile ("msr primask, %0" :: "r" (mask) : "memory");
}

void uart_init(int baud) {
    struct uart_reg_map *u = UART2_BASE;
    if (baud <= 0) return;
    nvic_irq(USART2_IRQ, IRQ_DISABLE);
    u->CR1 = 0u;
    tx.head = tx.tail = tx.count = 0u;
    rx.head = rx.tail = rx.count = 0u;

    struct rcc_reg_map *rcc = RCC_BASE;
    rcc->apb1_enr |= RCC_USART2EN;
    (void)rcc->apb1_enr; /* Read back after clock enable. */

    gpio_init(GPIO_A, 2, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT7);
    gpio_init(GPIO_A, 3, MODE_ALT, OUTPUT_OPEN_DRAIN, OUTPUT_SPEED_LOW, PUPD_NONE, ALT7);

    /* Oversampling by 16: BRR is the rounded peripheral clock / baud. */
    u->BRR = (UART_CLOCK_HZ + (uint32_t)baud / 2u) / (uint32_t)baud;
    u->CR2 = 0u; /* One stop bit. */
    u->CR3 = 0u;
    u->CR1 = USART_UE | USART_TE | USART_RE | USART_RXNEIE;
    /* TXEIE stays disabled until a byte is queued. */
    nvic_clear_pending(USART2_IRQ);
    nvic_irq(USART2_IRQ, IRQ_ENABLE);
}

int uart_put_byte(char c) {
    struct uart_reg_map *u = UART2_BASE;
    uint32_t mask = irq_save();
    int result = push(&tx, (uint8_t)c);
    if (result == 0 && !(u->CR1 & USART_TXEIE)) {
        u->CR1 |= USART_TXEIE;
    }
    irq_restore(mask);
    return result;
}

int uart_get_byte(char *c) {
    if (c == 0) return -1;
    struct uart_reg_map *u = UART2_BASE;
    uint8_t value;
    uint32_t mask = irq_save();
    int result = pop(&rx, &value);
    if (result == 0) {
        *c = (char)value;
        if (!(u->CR1 & USART_RXNEIE)) {
            u->CR1 |= USART_RXNEIE;
        }
    }
    irq_restore(mask);
    return result;
}

void USART2_TX_IRQHandler(void) {
    struct uart_reg_map *u = UART2_BASE;
    uint8_t value;
    if (pop(&tx, &value) == 0) {
        u->DR = value;  /* Write only: DR is not an RMW register. */
    }
    /* The last pending byte has been given to the peripheral. */
    if (tx.count == 0u) u->CR1 &= ~USART_TXEIE;
}

void USART2_RX_IRQHandler(void) {
    struct uart_reg_map *u = UART2_BASE;
    /* Read the received byte exactly once, clearing RXNE. */
    uint8_t value = (uint8_t)u->DR;
    (void)push(&rx, value);
    if (rx.count == UART_BUFFER_SIZE) u->CR1 &= ~USART_RXNEIE;
}

void USART2_IRQHandler(void) {
    struct uart_reg_map *u = UART2_BASE;
    uint32_t sr = u->SR;
    uint32_t cr1 = u->CR1;
    if ((sr & USART_RXNE) && (cr1 & USART_RXNEIE)) USART2_RX_IRQHandler();
    if ((sr & USART_TXE) && (cr1 & USART_TXEIE)) USART2_TX_IRQHandler();
    nvic_clear_pending(USART2_IRQ);
}

/* Only for sys_exit, after interrupts can no longer drain the queue. */
void uart_flush(void) {
    struct uart_reg_map *u = UART2_BASE;
    uint32_t mask = irq_save();
    u->CR1 &= ~USART_TXEIE;
    uint8_t value;
    while (pop(&tx, &value) == 0) {
        while (!(u->SR & USART_TXE)) { }
        u->DR = value;
    }
    while (!(u->SR & USART_TC)) { }
    irq_restore(mask);
}
