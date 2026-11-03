/* Initialize the user process stack, then enter the supplied crt0. */
.cpu cortex-m4
.syntax unified
.thumb
.section .text
.thumb_func
.global enter_user_mode
enter_user_mode:
    ldr r0, =__psp_stack_top
    msr psp, r0
    isb
    b _crt0
