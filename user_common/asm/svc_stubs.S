/**
 * @file   swi_stubs.S
 *
 * @brief  Stub functions for newlib and 349libc syscalls
 *
 * @date
 * @author
 */

.cpu cortex-m4
.syntax unified
.section .swi_stub
.thumb

#include "../../kernel/include/svc_num.h"

.global _start
_start:
  bkpt

.global _sbrk
_sbrk:
  svc SVC_SBRK
  bx lr

.global _write
_write:
  svc SVC_WRITE
  bx lr
  
.global _close
_close:
  bkpt

.global _fstat
_fstat:
  bkpt

.global _isatty
_isatty:
  bkpt

.global _lseek
_lseek:
  bkpt

.global _read
_read:
  svc SVC_READ
  bx lr

.global _gettimeofday
_gettimeofday:
  bkpt

.global _times
_times:
  bkpt

.global _getpid
_getpid:
  bkpt

.global _kill
_kill:
  bkpt

.global _exit
_exit:
  svc SVC_EXIT
  bx lr
  
.global servo_enable
servo_enable:
  bkpt

.global servo_set
servo_set:
  bkpt