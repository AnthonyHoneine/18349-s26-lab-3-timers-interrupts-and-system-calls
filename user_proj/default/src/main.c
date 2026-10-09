#include <stdio.h>
#include <unistd.h>

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

int main(UNUSED int argc, char const *argv[]){
  int status;
  int i = 0;
  while( 1 ){
    status = write(1, "User hello world!\n", 18);
    if (status < 18) {
      delay(status);
      printf("argv[1]: %s\n", argv[1]);
    }
    i++;
  }
  return 0;
}
