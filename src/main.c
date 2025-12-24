#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
  char *bltin[50]= {"exit", "echo", "type"};
  

while(1) {  
  // TODO: Uncomment the code below to pass the first stage
  printf("$ ");

  // Wait for user input
  char input[100];
  fgets(input, 100, stdin);

  // Remove the trailing newline
  input[strcspn(input, "\n")] = '\0';
  if (strcmp(input, "exit") == 0){
    break;
  }
  else if (strncmp(input, "echo",4) == 0){
    printf("%s\n", input + 5);
  }
  else if (strncmp(input, "type", 4) == 0){
    int flag = 0; 
    for(int i=0; i < 3; i++){
    
      if (strcmp(input+5, bltin[i]) == 0 ){
        flag = 1;
      }
    };
    if (flag == 1){
      printf("%s is a shell builtin\n", input +5);
    }
      else {
        printf("%s: not found\n", input +5);
      }
    }

  else{
  printf("%s: command not found\n", input);
  }
}
return 0;
}