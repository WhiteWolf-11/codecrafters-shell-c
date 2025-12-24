#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  // TODO: Uncomment the code below to pass the first stage
  while (1) {
   printf("$ ");

   scanf("%s", argv[0]);
   printf("%s: command not found\n", argv[0]);
  }
  return 0;
}
