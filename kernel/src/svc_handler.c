/**
 * @file 
 *
 * @brief      
 *
 * @date       
 *
 * @author     
 */

#include <stdint.h>
#include <debug.h>
#include "syscall.h"
#include <svc_num.h>

#define UNUSED __attribute__((unused))

void svc_c_handler(uint32_t *psp) {
  // uint32_t *psp = (uint32_t *)__get_PSP(); // prog man p59
  uint32_t banked_pc = psp[6];
  uint16_t instr = ((uint16_t *)banked_pc)[-1];
  uint8_t svc_number = instr & 0xff;
  switch ( svc_number ) {
    case SVC_SBRK:
      psp[0] = (uint32_t)sys_sbrk((int)psp[0]);
      break;
    case SVC_WRITE:
      psp[0] = (uint32_t)sys_write((int)psp[0], (char *)psp[1], (int)psp[2]);
      break;
    case SVC_READ:
      psp[0] = (uint32_t)sys_read((int)psp[0], (char *)psp[1], (int)psp[2]);
      break;
    case SVC_EXIT:
      sys_exit((int)psp[0]);
      break;
    case SVC_SERVO_ENABLE:
      psp[0] = (uint32_t)sys_servo_enable((uint8_t)psp[0], (uint8_t)psp[1]);
      break;
    case SVC_SERVO_SET:
      psp[0] = (uint32_t)sys_servo_set((uint8_t)psp[0], (uint8_t)psp[1]);
      break;
    default:
      DEBUG_PRINT( "Not implemented, svc num %d\n", svc_number );
      ASSERT( 0 );
  }
}