/** @file svc_handler.c @brief Dispatch SVC using hardware-saved PSP frame. */
#include <stdint.h>
#include <syscall.h>
#include <svc_num.h>
#include <debug.h>

void svc_c_handler(uint32_t *frame) {
    if (!frame) { for (;;) { __asm volatile ("bkpt #0"); } }
    /* Stacked PC is after the 16-bit Thumb SVC instruction. */
    uintptr_t pc = (uintptr_t)frame[6];
    uint16_t instruction = *(const uint16_t *)(pc - 2u);
    uint8_t number = (uint8_t)(instruction & 0xffu);
    if ((instruction & 0xff00u) != 0xdf00u) {
        for (;;) { __asm volatile ("bkpt #0"); }
    }
    switch (number) {
        case SVC_SBRK:
            frame[0] = (uint32_t)sys_sbrk((int32_t)frame[0]); break;
        case SVC_WRITE:
            frame[0] = (uint32_t)sys_write((int)frame[0], (char *)frame[1], (int)frame[2]); break;
        case SVC_READ:
            frame[0] = (uint32_t)sys_read((int)frame[0], (char *)frame[1], (int)frame[2]); break;
        case SVC_EXIT:
            sys_exit((int)frame[0]); break;
        case SVC_SERVO_ENABLE:
            frame[0] = (uint32_t)sys_servo_enable((uint8_t)frame[0], (uint8_t)frame[1]); break;
        case SVC_SERVO_SET:
            frame[0] = (uint32_t)sys_servo_set((uint8_t)frame[0], (uint8_t)frame[1]); break;
        default:
            frame[0] = (uint32_t)-1;
            break;
    }
}
