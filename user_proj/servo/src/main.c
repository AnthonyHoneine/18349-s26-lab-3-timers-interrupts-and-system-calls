
/** @file main.c @brief User-mode text command interface for dual servos. */
#include <stdint.h>
#include <unistd.h>
#include "../../../kernel/include/svc_num.h"

/* These wrappers issue SVC from unprivileged Thread mode. NEVER call the
 * kernel's sys_servo_* implementations directly from a user program. */
static int servo_enable_call(unsigned ch, unsigned on) {
    register uint32_t r0 __asm("r0") = ch;
    register uint32_t r1 __asm("r1") = on;
    __asm volatile ("svc %2" : "+r"(r0) : "r"(r1), "I"(SVC_SERVO_ENABLE) : "r2", "r3", "r12", "memory", "cc");
    return (int)r0;
}

static int servo_set_call(unsigned ch, unsigned angle) {
    register uint32_t r0 __asm("r0") = ch;
    register uint32_t r1 __asm("r1") = angle;
    __asm volatile ("svc %2" : "+r"(r0) : "r"(r1), "I"(SVC_SERVO_SET) : "r2", "r3", "r12", "memory", "cc");
    return (int)r0;
}

static const char *skip_spaces(const char *p) {
    while (*p == ' ' || *p == '\t') ++p;
    return p;
}

static int parse_uint(const char **p, unsigned *out) {
    const char *s = skip_spaces(*p);
    unsigned n = 0u;
    if (*s < '0' || *s > '9') return -1;
    while (*s >= '0' && *s <= '9') {
        if (n > 1000u) return -1;
        n = n * 10u + (unsigned)(*s++ - '0');
    }
    *out = n;
    *p = s;
    return 0;
}

static int match_word(const char **p, const char *word) {
    const char *s = skip_spaces(*p);
    while (*word && *s == *word) {
        ++s;
        ++word;
    }
    if (*word || (*s != ' ' && *s != '\t')) return -1;
    *p = s;
    return 0;
}

static int end_line(const char *p) {
    p = skip_spaces(p);
    return *p == '\0' || *p == '\r' || *p == '\n';
}

int main(int argc, const char *argv[]) {
    (void)argc;
    (void)argv;

    char line[64];

    static const char welcome[] =
        "Welcome to Servo Controller!\n"
        "Commands\n"
        "enable <ch>: Enable servo channel\n"
        "disable <ch>: Disable servo channel\n"
        "set <ch> <angle>: Set servo angle\n";

    write(STDOUT_FILENO, welcome, sizeof(welcome) - 1u);

    for (;;) {
        write(STDOUT_FILENO, "> ", 2);

        int n = (int)read(STDIN_FILENO, line, sizeof(line) - 1u);
        if (n <= 0) continue;

        line[n] = '\0';

        const char *p = line;
        unsigned ch, angle;
        int result = -1;

        if (match_word(&p, "enable") == 0 &&
            parse_uint(&p, &ch) == 0 &&
            ch < 2u && end_line(p)) {

            result = servo_enable_call(ch, 1u);

        } else {
            p = line;

            if (match_word(&p, "disable") == 0 &&
                parse_uint(&p, &ch) == 0 &&
                ch < 2u && end_line(p)) {

                result = servo_enable_call(ch, 0u);

            } else {
                p = line;

                if (match_word(&p, "set") == 0 &&
                    parse_uint(&p, &ch) == 0 &&
                    ch < 2u &&
                    parse_uint(&p, &angle) == 0 &&
                    angle <= 180u && end_line(p)) {

                    result = servo_set_call(ch, angle);
                }
            }
        }

        if (result != 0) {
            static const char error_msg[] = "Invalid command\n";
            write(STDOUT_FILENO, error_msg, sizeof(error_msg) - 1u);
        }
    }
}
