/**
 * @file main.c
 * @brief User-mode interface for controlling two servos.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <servo.h>

#define UNUSED __attribute__((unused))

int main(UNUSED int argc, UNUSED const char *argv[]) {
    char buf[64];
    char command[16], extra;
    unsigned int channel, angle;
    int result;

    printf("Welcome to Servo Controller!\n"
           "Commands\n"
           "enable <ch>: Enable servo channel\n"
           "disable <ch>: Disable servo channel\n"
           "set <ch> <angle>: Set servo angle\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        int len = read(0, buf, sizeof(buf) - 1);
        if (len <= 0) continue;

        buf[len] = '\0';
        result = -1;

        if (sscanf(buf, "%15s", command) != 1) {
            printf("Invalid command\n");
            continue;
        }

        if (strcmp(command, "enable") == 0) {
            if (sscanf(buf, "%15s %u %c", command, &channel, &extra) == 2 && channel < 2) {
                result = servo_enable(channel, 1);
            }
        } else if (strcmp(command, "disable") == 0) {
            if (sscanf(buf, "%15s %u %c", command, &channel, &extra) == 2 && channel < 2) {
                result = servo_enable(channel, 0);
            }
        } else if (strcmp(command, "set") == 0) {
            if (sscanf(buf, "%15s %u %u %c", command, &channel, &angle, &extra) == 3 &&
                channel < 2 && angle <= 180) {
                result = servo_set(channel, angle);
            }
        }

        if (result != 0) printf("Invalid command\n");
    }
}
