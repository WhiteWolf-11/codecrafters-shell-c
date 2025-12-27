#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


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
    char *command = input + 5;
    int flag = 0; 
    for(int i=0; i < 3; i++){
    
      if (strcmp(input+5, bltin[i]) == 0 ){
        flag = 1;
      }
    };
    if (flag == 1){
      printf("%s is a shell builtin\n", command);
    }
      else {
        char *path = getenv("PATH");
        char *path_copy = strdup(path);
        char *dir = strtok(path_copy, ":");
        char file_path[200];
        while(dir != NULL){

          snprintf(file_path, sizeof(file_path), "%s/%s", dir, command);
          if(access(file_path, X_OK) == 0){
            flag = 1;
            break;
          }
           dir = strtok(NULL, ":");
        }
        free(path_copy);
        if (flag == 1){
          printf("%s is %s\n", command, file_path);
        }
        else {
          printf("%s: not found\n", command);
      }
    }
  }

  else{
  printf("%s: command not found\n", input);
  }
}
return 0;
}