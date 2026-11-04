/**
 * @file syscall.c
 * @brief Kernel implementations of system calls.
 */

#include <stdint.h>
#include <unistd.h>
#include <syscall.h>
#include <uart.h>
#include <printk.h>
#include <lcd_driver.h>

extern char __heap_low;
extern char __heap_top;

void *sys_sbrk(int incr) {
    static uintptr_t heap_break = 0; // static so it persists

    if (heap_break == 0) heap_break = (uintptr_t)&__heap_low;

    int64_t next = (int64_t)heap_break + incr;

    if (next < (uintptr_t)&__heap_low || next > (uintptr_t)&__heap_top)
        return (void *)-1;

    void *previous = (void *)heap_break;
    heap_break = next;

    return previous;
}

int sys_write(int file, char *ptr, int len) {
    if (file != STDOUT_FILENO || len < 0) return -1;

    for (int i = 0; i < len; i++) {
        while (uart_put_byte(ptr[i]) != 0);
    }
    return len;
}

int sys_read(int file, char *ptr, int len) {
    if (file != STDIN_FILENO || len < 0) return -1;

    static int skip_lf = 0;
    int n = 0;
    char c;

    while (n < len) {
        while (uart_get_byte(&c) != 0);

        // Ignore the LF following a CR from Windows terminals
        if (skip_lf) {
            skip_lf = 0;
            if (c == '\n') continue;
        }

        if (c == 4) {
            return n;
        } else if (c == '\b' || c == 127) {
            if (n > 0) {
                n--;
                sys_write(STDOUT_FILENO, "\b \b", 3);
            }
        } else if (c == '\r' || c == '\n') {
            skip_lf = (c == '\r');
            ptr[n++] = '\n';
            sys_write(STDOUT_FILENO, "\r\n", 2);
            return n;
        } else {
            ptr[n++] = c;
            sys_write(STDOUT_FILENO, &c, 1);
        }
    }

    return n;
}

void sys_exit(int status) {
    printk("Program finished with exit code %d\n", status);
    uart_flush();

    char text[12];
    int i = sizeof(text) - 1;
    text[i] = '\0';

    unsigned value = status;
    if (status < 0) value = -value;
    if (value == 0) text[--i] = '0';

    while (value > 0) {
        text[--i] = '0' + value % 10;
        value /= 10;
    }

    if (status < 0) text[--i] = '-';

    lcd_clear();
    lcd_print(&text[i]);

    __asm volatile("cpsid i");
    while (1) __asm volatile("wfi");
}
