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

#define UNUSED __attribute__((unused))

void svc_c_handler(uint32_t *psp) {
  // uint32_t *psp = (uint32_t *)__get_PSP(); // prog man p59
  uint32_t banked_pc = psp[6];
  uint16_t instr = ((uint16_t *)banked_pc)[-1];
  switch ( svc_number ) {
    case 0:
      psp[0] = (uint32_t)sys_sbrk((int)psp[0]);
      break;
    case 1:
      psp[0] = (uint32_t)sys_write((int)psp[0], (char *)psp[1], (int)psp[2]);
      break;
    case 6:
      psp[0] = (uint32_t)sys_read((int)psp[0], (char *)psp[1], (int)psp[2]);
      break;
    case 7:
      sys_exit((int)psp[0]);
      break;
    default:
      DEBUG_PRINT( "Not implemented, svc num %d\n", svc_number );
      ASSERT( 0 );
  }
}