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


/**
 * Given a vector which contains arguments given for a command, reduce it to 'real' arguments (not empty strings produced by splitting on whitespace)
 */
void trim_args(vector * args) {
    int arg_idx = vector_size(args) - 1;
    char * current;
    for (; arg_idx >= 0; arg_idx--) {
        current = vector_get(args, arg_idx);
        if (!strcmp(current, "")) {
             vector_erase(args, arg_idx);
        }
       
    }
}


/**
 * Given a string 'command', separate it into multiple arguments
 */
char ** parse_command(char * command) {
    sstring * command_wrapper = cstr_to_sstring(command); //wrap command as sstring to use split
    vector *  args = sstring_split(command_wrapper, ' '); //separate by spaces
    trim_args(args);    //remove empty strings created by split on whitespace

    size_t arg_count = vector_size(args);

    size_t bytes_needed = sizeof(char *) * arg_count;

    char ** command_args = malloc(bytes_needed + sizeof(void *));  
    
    size_t arg_idx = 0;
    for (; arg_idx < arg_count; arg_idx++) {
        command_args[arg_idx] = vector_get(args, arg_idx);
    }
    command_args[arg_count + 1] = NULL; 
    
    return command_args;
}

void exec_command(char * command) {
    char ** command_argv = parse_command(command);

    pid_t child_pid = fork();

    if (child_pid < 0) {
        //fork failed
        print_fork_failed();
        return;
    } else if (child_pid == 0) {
        //child process
        pid_t my_pid = getpid();
        print_command_executed(my_pid); //command exec'd by <pid>
        execvp(command_argv[0], command_argv);
        print_exec_failed(command);
        exit(1);
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
