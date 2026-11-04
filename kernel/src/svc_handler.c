/**
 * @file svc_handler.c
 * @brief Handles system calls from user mode.
 */

#include <stdint.h>
#include <syscall.h>
#include <svc_num.h>

void svc_c_handler(uint32_t *frame) {
    // Get the SVC number from the instruction before the saved PC
    uint16_t instruction = *(uint16_t *)(frame[6] - 2);
    uint8_t svc_num = instruction & 0xFF;

    switch (svc_num) {
        case SVC_SBRK:
            frame[0] = (uint32_t)sys_sbrk((int)frame[0]);
            break;
        case SVC_WRITE:
            frame[0] = sys_write(frame[0], (char *)frame[1], frame[2]);
            break;
        case SVC_READ:
            frame[0] = sys_read(frame[0], (char *)frame[1], frame[2]);
            break;
        case SVC_EXIT:
            sys_exit(frame[0]);
            break;
        case SVC_SERVO_ENABLE:
            frame[0] = sys_servo_enable(frame[0], frame[1]);
            break;
        case SVC_SERVO_SET:
            frame[0] = sys_servo_set(frame[0], frame[1]);
            break;
        default:
            frame[0] = -1;
            break;
    }
}
