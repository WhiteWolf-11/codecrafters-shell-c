#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>

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

        while (*p != '\0' && *p != ' ') {
            if (*p == '\\') {
                // Outside quotes: Always escape next character
                p++; 
                if (*p != '\0') arg[j++] = *p++;
            } 
            else if (*p == '\'') {
                // Single Quotes: Everything is literal
                p++; 
                while (*p != '\0' && *p != '\'') arg[j++] = *p++;
                if (*p == '\'') p++;
            } 
            else if (*p == '\"') {
                // Double Quotes: Selective escaping
                p++; 
                while (*p != '\0' && *p != '\"') {
                    if (*p == '\\') {
                        char next = *(p + 1);
                        // Only escape if next is " or 
                        if (next == '\"' || next == '\\' || next == '$') {
                            p++; // Skip the backslash
                            arg[j++] = *p++; // Copy the escaped char
                        } else {
                            arg[j++] = *p++; // Copy backslash literally
                        }
                    } else {
                        arg[j++] = *p++;
                    }
                }
                if (*p == '\"') p++;
            } 
            else {
                arg[j++] = *p++;
            }
        }
        arg[j] = '\0';
    }
    argv[i] = NULL;
}

int get_redirect_type(char *arg) {
    if (strcmp(arg, ">") == 0 || strcmp(arg, "1>") == 0) return 1;  // Stdout Truncate
    if (strcmp(arg, "2>") == 0) return 2;                         // Stderr Truncate
    if (strcmp(arg, ">>") == 0 || strcmp(arg, "1>>") == 0) return 3; // Stdout Append
    if (strcmp(arg, "2>>") == 0) return 4;                        // Stderr Append
    return 0; // Not a redirection
}

struct termios original_termios;

void disable_raw_mode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void enable_raw_mode(){
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(disable_raw_mode);
    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    setbuf(stdout, NULL);
    char *argv[20];
    char res[1024];
    char *bltin[] = {"exit", "echo", "type", "pwd", "cd"};
    int target_fd = 1;
    char *autocomplete_targets[] = {"echo ", "exit "};
    

    while (1) {

        printf("$ ");
        char input[1024];
        int pos = 0;

        enable_raw_mode();
        while(1){
            char c;
            if(read(STDIN_FILNO, &c, 1) != 1) break;
            if(c == '\n'){
                printf("\n");
                break;
            }else if (c == '\t'){
                for(int i = 0; i < 2; i++){
                    if(pos > 0 && strncmp(autocomplete_targets[i], input, pos) == 0){
                        printf("%s\n", &autocomplete_targets[i][pos]);
                        strcpy(&input[pos], &autocomplete_targets[i][pos]);
                        pos = strlen(input);
                        break;
                    }

                }
            }else if(c == 127){ // Backspace
                if(pos > 0){
                    pos--;
                    input[pos] = '\0';
                    printf("\b \b");
                }
        }else{
                input[pos++] = c;
                input[pos] = '\0';
                printf("%c", c);
            }
            fflush(stdout);
        }
        disable_raw_mode();

        input[strcspn(input, "\n")] = '\0';
        parse_input(input, argv);
        if (argv[0] == NULL) continue;

        int fd = -1;
        int redirect_idx = -1;
    
        for (int k = 0; argv[k]!= NULL; k++){
            char *arg = argv[k];
            if(strchr(arg, '>') != NULL){
                target_fd = (arg[0] == '2') ? 2 : 1; // Determine target fd
                int flags = O_WRONLY | O_CREAT | (strstr(arg, ">>") ? O_APPEND : O_TRUNC);
                if (argv[k+1]!= NULL){
                   redirect_idx = k;
                   fd = open(argv[k+1], flags, 0644);
                   argv[k] = NULL;
                   break;
                }
            }
        }

        if (strcmp(argv[0], "exit") == 0) {
            if(fd != -1) close(fd);
            for(int k = 0; argv[k] != NULL; k++) {
                free(argv[k]);
            }
            break;
        }
        else if (strcmp(argv[0], "cd") == 0) {
            char *path = argv[1];
            if (!path || strcmp(path, "~") == 0) path = getenv("HOME");
            if (chdir(path) == 0) {
                char cwd[1024];
                if (getcwd(cwd, sizeof(cwd))) setenv("PWD", cwd, 1);
            } else {
                fprintf(stderr, "cd: no such file or directory: %s\n", path);
            }
            if (fd != -1) close(fd);
        }  
        else{
            pid_t pid = fork();
            if (pid == 0) {
                if (fd != -1) {
                    dup2(fd, target_fd);
                    close(fd);
                }
                // Execute other commands in child process
                if (strcmp(argv[0], "echo") == 0) {
                    for (int j = 1; argv[j] != NULL; j++) {
                        printf("%s%s", argv[j], (argv[j+1] == NULL) ? "" : " ");
                    }
                    printf("\n");
                    exit(0);
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
                        else fprintf(stderr, "%s: not found\n", argv[1]);
                    }
                    exit(0);
                } 
                else if (strcmp(argv[0], "pwd") == 0) {
                    char cwd[1024];
                    if (getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
                    exit(0);
                } 
                else {
                    // EXTERNAL COMMANDS (ls, cat, etc.)
                    int found = 0;
                    char *path = NULL;
                    if (strchr(argv[0], '/') != NULL) {
                        if (access(argv[0], X_OK) == 0) { found = 1; path = argv[0]; }
                    } else {
                        path = file_path(argv[0], res, &found);
                    }

                    if (found) {
                        execv(path, argv);
                        perror("execv"); // Only runs if execv fails
                    } else {
                        fprintf(stderr, "%s: command not found\n", argv[0]);
                    }
                    exit(found ? 0 : 1);
                }
            } else { // PARENT
                if (fd != -1) close(fd);
                wait(NULL);
            }
        }

        // Cleanup argv memory
        for (int k = 0; argv[k] != NULL; k++) {
            free(argv[k]);
            argv[k] = NULL;
        }
    }
    return 0;
}