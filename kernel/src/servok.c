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
#include <syscall.h>
#include <gpio.h>

#define UNUSED __attribute__((unused))

volatile uint8_t servo_active[2] = {0, 0}; 
volatile uint8_t servo_angle[2] = {0, 0};

int sys_servo_enable(uint8_t channel, uint8_t enabled){
  if (channel > 1) return -1;

  servo_active[channel] = enabled;
  if (!enabled) {
    if (channel == 0) {
      gpio_clr(GPIO_A, 1);
    } else {
      gpio_clr(GPIO_B, 0);
    }
  }
  return 0;
}

int sys_servo_set(uint8_t channel, uint8_t angle){
  if (channel > 1) return -1;

  servo_angle[channel] = angle;
  return 0;
}