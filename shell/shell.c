/**
 * Shell
 * CS 241 - Fall 2019
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include "sstring.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct process {
    char *command;
    pid_t pid;
} process;

static vector * history;

void exec_command(char * command) {
    pid_t child_pid = fork();

    if (child_pid < 0) {
        //fork failed
        print_fork_failed();
        return;
    } else if (child_pid == 0) {
        //child process
        pid_t my_pid = getpid();
        print_command_executed(my_pid); //command exec'd by <pid>
    } else {
        int status;
        waitpid(child_pid, &status, 0);
    }

    
}
int shell(int argc, char *argv[]) {
    history = string_vector_create();

    // TODO: This is the entry point for your shell.
    pid_t outer_pid = getpid();
    char cwd[1024]; //enforce char limit of 4096 on path
    char line[1024];
    //size_t char_limit = 1024;
    while (1) {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            print_prompt(cwd, outer_pid);
        }

        
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        };

        //terminate command with \0 to make it proper c-string
        char * final_char = strchr(line, '\n');
        if (final_char) {
            *final_char = '\0';
        }
        
        exec_command(line);

        break;
    }   
    return 0;
}
