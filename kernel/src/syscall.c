/**
 * @file syscall.c
 *
 * @brief      
 *
 * @date       
 *
 * @author     
 */

#include <unistd.h>
#include <syscall.h>
#include <uart.h>
#include <printk.h>
#include <stdio.h>
#include <lcd_driver.h>

#define UNUSED __attribute__((unused))

extern char __heap_low;
extern char __heap_top;
void *sys_sbrk(int incr){
  static char *current_break = 0;
  char *prev_break;

  if (current_break == 0) current_break = &__heap_low;
  prev_break = current_break;

  if (current_break + incr > &__heap_top) return (void*)-1;

  current_break += incr;
  return (void *)prev_break;
}

int sys_write(int file, char *ptr, int len){
  if (file != 1) return -1;
  for (int i = 0; i < len; i++) {
    while (uart_put_byte(ptr[i]) != 0);
  }
  return 0;
}

int sys_read(int file, char *ptr, int len){
  if (file != 0) return -1; // not read
  int bytes_read = 0;
  char c;

  while (bytes_read < len) {
    while (uart_get_byte(&c) != 0);

    switch (c) {
    case 4:
      return bytes_read;
    case '\b':
      if (bytes_read > 0) {
        bytes_read--;
        uart_put_byte('\b');
        uart_put_byte(' ');
        uart_put_byte('\b');
      }
      break;
    case '\r': //fall through
    case '\n':
      bytes_read++;
      ptr[bytes_read] = '\n';
      uart_put_byte('\n');
      return bytes_read;
    default:
      bytes_read++;
      ptr[bytes_read] = c;
      uart_put_byte(c);
      break;
    }
  }
  return bytes_read;
}

void sys_exit(int status){
  // print exit code
  printk("Programmed finished with exit code %d", status);
  // flush uart buffer
  uart_flush();
  // display exit code on lcd screen
  char exit_string[12]; 
  snprintf(exit_string, sizeof(exit_string), "%d", status);
  lcd_print(exit_string);
  // sleep with interrupts disabled
  __asm volatile("cpsid i");
  while (1) __asm volatile("wfi");
}