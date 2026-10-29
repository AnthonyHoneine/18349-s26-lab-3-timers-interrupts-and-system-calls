#include <stdio.h>
#include <unistd.h>
#include "../../../kernel/include/arm.h"
#include "../../../kernel/include/syscall.h"
#include "../../../kernel/include/servok.h"

#define TRUE 1
#define FALSE 0

#define UNUSED __attribute__((unused))

/** @brief  Executes a fairly long delay. */
int delay(int status) {
  int x = 18;
  for (int i = 0; i < 100000; i++) {
    x = x + 349;
  }
  printf("\nBytes last printed: %d\n", status);
  return x;
}

int main(UNUSED int argc, UNUSED char const *argv[]){
  char buf[16];
  // int status;
  int channel, angle;
  printf("Welcome to Servo Controller!\n"
          "Commands\n"
          "enable <ch>: Enable servo channel\n"
          "disable <ch>: Disable servo channel\n"
          "set <ch> <angle>: Set servo angle\n");
  while( 1 ){
    int len = sys_read(0, buf, sizeof(buf) - 1);
    if (len > 0) {
      buf[len] = '\0';
      
      if (sscanf(buf, "enable %d", &channel) == 1) {
        sys_servo_enable(channel, TRUE);
      } else if (sscanf(buf, "disable %d", &channel) == 1) {
        sys_servo_enable(channel, FALSE);
      } else if (sscanf(buf, "set %d %d", &channel, &angle) == 1) {
        sys_servo_set(channel, angle);
      } else {
        printf("Invalid command\n");
      }
    }
    // status = write(1, "User hello world!\n", 18);
    // if (status < 18) {
    //   delay(status);
    //   printf("argv[1]: %s\n", argv[1]);
    // }
    // i++;
  }
  return 0;
}
