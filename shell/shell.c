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
static char * history_file_name;
static char * history_fp;
static FILE * history_file;



void initialize_history_from_file() {
    history_file = fopen(history_file_name, "r");
    char line[1024];
    while (fgets(line, sizeof(line), history_file)) {
        vector_push_back(history, line);
    }

    fclose(history_file);
}

void check_history_file(char * opt_arg) {
    if (opt_arg && !history_file_name) {
        history_file_name = strdup(opt_arg); //must duplicate optarg b/c it changes on getopt call
        history_fp = get_full_path(history_file_name); //get full path in case cd is called later
    }

    //since this function is called on shell entry, we can assume cwd == history file location
    int file_exists = history_file_name && (access(history_file_name, R_OK | W_OK) != -1);
    if (file_exists) {
        initialize_history_from_file();
    } else {
        history_file = fopen(history_file_name, "w"); //open the history file in write mode to create it
        fclose(history_file);
    }
}
/**
 * Checks for + handles optional args in initial shell call
 */ 
void parse_options(int argc, char * argv[]) {
    char * opt_string = "h:f:";
    int opt_char;
    //initialziing opt_char and ensuring ret value > -1 (i.e. option character was found)
    while((opt_char = getopt(argc, argv, opt_string)) != -1) {
        switch(opt_char) {
            case 'h':
                check_history_file(optarg);
                break;
        }
    }
}

/**
 * Add a command to history.
 * TODO: add toggle for writing to file versus only storing in vector
*/
void record_command(char * command) {
    vector_push_back(history, command);
    if (history_file_name) {
        history_file = fopen(history_fp, "a");
        fprintf(history_file, "%s\n", command);
        fclose(history_file);
    }
}

/**
 * Given arguments vector, remove any empty strings caused by splitting on whitespace
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
 * Given a string 'command', separate it into distinct arguments and place in string array
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

/**
 * Execute the given command.
 * TODO: add toggle between execution of built-in commands and external commands
 * TODO: add support for backgrounding
 */ 

void exec_command(char * command) {
    record_command(command); //add command to history
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
    parse_options(argc, argv);

    // TODO: This is the entry point for your shell.
    pid_t outer_pid = getpid();
    char cwd[1024]; //enforce char limit on path
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
