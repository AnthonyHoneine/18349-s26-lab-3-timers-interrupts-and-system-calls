/** @file syscall.c @brief System calls for the serial console and user heap. */
#include <stdint.h>
#include <unistd.h>
#include <syscall.h>
#include <uart.h>
#include <printk.h>
#include <lcd_driver.h>

extern char __heap_low;
extern char __heap_top;

void *sys_sbrk(int incr) {
    static uintptr_t brk = 0u;
    uintptr_t lo = (uintptr_t)&__heap_low;
    uintptr_t hi = (uintptr_t)&__heap_top;
    if (!brk) brk = (lo + 7u) & ~(uintptr_t)7u;
    if (brk > hi) return (void *)-1;
    uintptr_t old = brk;
    if (incr >= 0) {
        if ((uintptr_t)incr > hi - brk) return (void *)-1;
        brk += (uintptr_t)incr;
    } else {
        uintptr_t decrease = (uintptr_t)(-(int64_t)incr);
        if (decrease > brk - ((lo + 7u) & ~(uintptr_t)7u)) return (void *)-1;
        brk -= decrease;
    }
    return (void *)old;
}

/* UART has higher exception priority than SVC (init_349), so its ISR may
 * preempt these kernel calls and empty/fill the buffers. */
static void send_byte(char c) {
    while (uart_put_byte(c) != 0) { /* Await room in TX queue. */ }
}

int sys_write(int file, char *ptr, int len) {
    if (file != 1 || len < 0 || (!ptr && len)) return -1;
    for (int i = 0; i < len; i++) send_byte(ptr[i]);
    return len;
}

int sys_read(int file, char *ptr, int len) {
    if (file != 0 || len < 0 || (!ptr && len)) return -1;
    int n = 0;
    char c;
    static uint8_t skip_lf_after_cr = 0u;
    while (n < len) {
        while (uart_get_byte(&c) != 0) { /* Await RX interrupt. */ }
        if (skip_lf_after_cr) {
            skip_lf_after_cr = 0u;
            if (c == '\n') continue;
        }
        if (c == 4) return n;
        if (c == '\b' || c == 127) {
            if (n) {
                --n;
                send_byte('\b'); send_byte(' '); send_byte('\b');
            }
            continue;
        }
        if (c == '\r' || c == '\n') {
            skip_lf_after_cr = (c == '\r');
            ptr[n++] = '\n';
            send_byte('\r'); send_byte('\n');
            return n;
        }
        ptr[n++] = c;
        send_byte(c);
    }
    return n;
}

void sys_exit(int status) {
    printk("Program finished with exit code %d\n", status);
    uart_flush();
    /* Avoid calling printf/snprintf from inside an SVC exception. */
    lcd_clear();
    char text[16];
    uint32_t val = (status < 0) ? (uint32_t)(-(int64_t)status) : (uint32_t)status;
    unsigned n = 0;
    do { text[n++] = (char)('0' + val % 10u); val /= 10u; } while (val && n < 14u);
    if (status < 0) text[n++] = '-';
    char out[16];
    unsigned i = 0;
    while (n) out[i++] = text[--n];
    out[i] = '\0';
    lcd_print(out);
    __asm volatile ("cpsid i" ::: "memory");
    for (;;) __asm volatile ("wfi");
}
