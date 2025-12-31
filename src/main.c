#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Helper to find command in PATH
char *file_path(char *command, char *res, int *flag) {
    char *path = getenv("PATH");
    if (!path) return NULL;
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
    return (*flag == 1) ? res : NULL;
}

// Function to handle single quotes and spaces
void parse_input(char *input, char **argv) {
    int i = 0;
    char *p = input;
    
    while (*p != '\0') {
        while (*p == ' ') p++; // Skip leading spaces
        if (*p == '\0') break;

        char *arg = malloc(1024); 
        int j = 0;
        argv[i++] = arg;

        while (*p != '\0' && (*p != ' ' )) {
          if (*p == '\\') {
                p++; // Skip the backslash
                if (*p != '\0') {
                    arg[j++] = *p++; // Copy the next character literally
                }
            } else if (*p == '\'') {
                p++; // Skip opening quote
                while (*p != '\0' && *p != '\'') {
                    arg[j++] = *p++; // Copy inside quotes literally
                }
                if (*p == '\'') p++; // Skip closing quote
            } else if (*p == '\"') {
                p++; // Skip opening double quote
                while (*p != '\0' && *p != '\"') {
                    arg[j++] = *p++; // Copy literally
                }
                if (*p == '\"') p++; // Skip closing double quote
            }
            else {
                arg[j++] = *p++; // Normal character
            }
        }
        arg[j] = '\0';
    }
    argv[i] = NULL;
}

int main() {
    setbuf(stdout, NULL);
    char *argv[20];
    char res[1024];
    char *bltin[] = {"exit", "echo", "type", "pwd", "cd"};

    while (1) {
        printf("$ ");

        char input[100];
        if (fgets(input, 100, stdin) == NULL) break;

        input[strcspn(input, "\n")] = '\0';
        
        // Use the new parser instead of strtok
        parse_input(input, argv);

        if (argv[0] == NULL) continue;

        if (strcmp(argv[0], "exit") == 0) {
            break;
        } 
        else if (strcmp(argv[0], "echo") == 0) {
            for (int j = 1; argv[j] != NULL; j++) {
                printf("%s%s", argv[j], (argv[j+1] == NULL) ? "" : " ");
            }
            printf("\n");
        } 
        else if (strcmp(argv[0], "type") == 0) {
            int flag = 0;
            for (int k = 0; k < 5; k++) {
                if (argv[1] && strcmp(argv[1], bltin[k]) == 0) {
                    flag = 1;
                }
            }
            if (flag == 1) {
                printf("%s is a shell builtin\n", argv[1]);
            } else if (argv[1]) {
                int found = 0;
                file_path(argv[1], res, &found);
                if (found) printf("%s is %s\n", argv[1], res);
                else printf("%s: not found\n", argv[1]);
            }
        } 
        else if (strcmp(argv[0], "pwd") == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
        } 
        else if (strcmp(argv[0], "cd") == 0) {
            char *path = argv[1];
            if (!path || strcmp(path, "~") == 0) path = getenv("HOME");
            if (chdir(path) == 0) {
                char cwd[1024];
                if (getcwd(cwd, sizeof(cwd))) setenv("PWD", cwd, 1);
            } else {
                printf("cd: no such file or directory: %s\n", path);
            }
        } 
        else {
            // External Command Logic
            int found_executable = 0;
            char *full_path_to_run = NULL;
            if (strchr(argv[0], '/') != NULL) {
                if (access(argv[0], X_OK) == 0) {
                    found_executable = 1;
                    full_path_to_run = argv[0];
                }
            } else {
                file_path(argv[0], res, &found_executable);
                full_path_to_run = res;
            }

            if (found_executable) {
                if (fork() == 0) {
                    execv(full_path_to_run, argv);
                    perror("execv");
                    exit(1);
                } else {
                    wait(NULL);
                }
            } else {
                printf("%s: command not found\n", argv[0]);
            }
        }

        // Clean up memory from parse_input
        for (int k = 0; argv[k] != NULL; k++) {
            free(argv[k]);
            argv[k] = NULL;
        }
    }
    return 0;
}