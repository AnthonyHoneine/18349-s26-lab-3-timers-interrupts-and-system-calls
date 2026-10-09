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

#define UNUSED __attribute__((unused))

int sys_servo_enable(UNUSED uint8_t channel, UNUSED uint8_t enabled){
  return -1;
}

int sys_servo_set(UNUSED uint8_t channel, UNUSED uint8_t angle){
  return -1;
}