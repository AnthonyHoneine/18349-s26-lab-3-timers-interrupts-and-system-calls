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
  bkpt

.global _write
_write:
  bkpt

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
  bkpt

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
  bkpt

.global servo_enable
servo_enable:
  bkpt

.global servo_set
servo_set:
  bkpt