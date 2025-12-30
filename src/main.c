#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


char *file_path(char *command,char *res, int *flag) {
  char *path = getenv("PATH");
  char *path_copy = strdup(path);
  char *dir = strtok(path_copy, ":");
  while (dir != NULL) {
    snprintf(res, 1024, "%s/%s", dir, command);
    if (access(res, X_OK) == 0) {
      *flag = 1;
      break;
    }
    dir = strtok(NULL, ":");
  }

  free(path_copy);
  (*flag == 1) ? res : NULL;
  return res;
}


int main() {
  // Flush after every printf
  setbuf(stdout, NULL);
  char *argv[20];
  char res[1024];
  char *bltin[50] = {"exit", "echo", "type"};

  while (1) {
    // TODO: Uncomment the code below to pass the first stage
    printf("$ ");

    // Wait for user input
    char input[100];
    if (fgets(input, 100, stdin) == NULL)
      break;
    int i = 0;

    // Remove the trailing newline
    input[strcspn(input, "\n")] = '\0';
    char *token = strtok(input, " ");

    while (token != NULL && i < 19) {
      argv[i++] = token;
      token = strtok(NULL, " ");
    }
    argv[i] = NULL;
    char *command = argv[0];
    if (argv[0] == NULL)
      continue;

    if (strcmp(argv[0], "exit") == 0) {
      break;
    } else if (strncmp(argv[0], "echo", 4) == 0) {
      printf("%s\n", argv[1]);
    } else if (strncmp(argv[0], "type", 4) == 0) {
      int flag = 0;
      for (int i = 0; i < 3; i++) {
        if (strcmp(argv[1], bltin[i]) == 0) {
          flag = 1;
        }
      };
      if (flag == 1) {
        printf("%s is a shell builtin\n", argv[1]);
      } else {
        char resolved_path[1024];
        file_path(argv[1], res, &flag);
        if (flag == 1) {
          printf("%s is %s\n", argv[1], res);
        } else {
          printf("%s: not found\n", argv[1]);
        }
      }
    } else {
      int found_executable = 0;
      // Search for the command in PATH
      file_path(argv[0], res, &found_executable);

      if (found_executable == 1) {
        pid_t pid = fork();
        if (pid == 0) {
          // Child process: execute the found path with the original argv
          if (execv(res, (char * const*)argv) == -1) {
            perror("execv");
            exit(1);
          }
        } else if (pid < 0) {
          perror("fork");
        } else {
          // Parent process: wait for the command to finish
          wait(NULL);
        }
      } else {
        // If not found, print ONLY the command name followed by : command not found
        // The tester is very sensitive to spaces and extra characters!
        printf("%s: command not found\n", argv[0]);
      }
    }
  }

  return 0;
}
