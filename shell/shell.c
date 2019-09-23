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
#include <errno.h>
typedef struct process {
    char *command;
    pid_t pid;
} process;

static vector * history;
static char * history_file_name;
static char * history_fp;
static FILE * history_file;

static char * script_file_name;
static char * script_fp;
static FILE * script_file;

/**
 * Reads command history from file (if it wasn't created by shell) and stores in internal vector.
 */ 
void initialize_history_from_file() {
    history_file = fopen(history_file_name, "r");
    char line[1024];
    while (fgets(line, sizeof(line), history_file)) {
        vector_push_back(history, line);
    }

    fclose(history_file);
}

/**
 * 1) Establishes file at given filepath as program history file.
 * 2) If the file exists, copy its value to internal history record.
 * 3) If the file does not exist, create the file.
 */
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
        print_history_file_error();
        history_file = fopen(history_file_name, "w"); //open the history file in write mode to create it
        if (!history_file) {
            perror("Error creating history file.");
            exit(1);
        }
        fclose(history_file);
    }
}

/**
 * Verifies that the file at the given path exists.
 * Establishes it as instruction queue and does set-up for execution 
 */
void check_script_file(char * opt_arg) {
    if (opt_arg && !script_file_name) {
        script_file_name = strdup(opt_arg);
        script_fp = get_full_path(script_file_name);
    }

    int file_exists = script_file_name && (access(script_file_name, R_OK) != -1);
    if (!file_exists) {
        print_script_file_error();
        exit(1);
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
            case 'f':
                check_script_file(optarg);
                break;
            default:
                print_usage();
                exit(1);
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
        if (history_file) {
            fprintf(history_file, "%s\n", command);
            fclose(history_file);
        }
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


void main_loop() {
    if (script_file_name) {
        script_file = fopen(script_fp, "r");
        if (!script_file) {
            print_script_file_error();
            exit(1);
        }

        FILE * command_src = script_file ? script_file : stdin;

        pid_t shell_pid  = getpid();
        char cwd[1024];
        char line[1024];
        while (1) {
            
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                print_prompt(cwd, shell_pid);
            }

            if (!fgets(line, sizeof(line), command_src)) {
                break; //TODO: cleanup functions
            }

            char * final_char = strchr(line, '\n');
            if (final_char) {
                *final_char = '\0';
            }

            if (command_src == script_file) {
                print_command(line);
            }

            exec_command(line);


        }

        fclose(script_file);
    }
}
int shell(int argc, char *argv[]) {
    history = string_vector_create();
    parse_options(argc, argv);

    main_loop();

    return 0;
}
