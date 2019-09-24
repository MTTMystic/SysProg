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
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef struct process {
    char *command;
    pid_t pid;
} process;

static vector * history = NULL;
static char * history_file_name = NULL;
static char * history_fp = NULL;
static FILE * history_file = NULL;

static char * script_file_name = NULL;
static char * script_fp = NULL;
static FILE * script_file = NULL;

#define close_shell_err cleanup(); exit(EXIT_FAILURE);
#define close_shell cleanup(); exit(EXIT_SUCCESS);


void cleanup() {
    while (waitpid((pid_t) (-1), 0, WNOHANG) > 0);
    
    vector_destroy(history);

    if (history_fp) {
        free(history_fp);
    }

    if (script_fp) {
        free(script_fp);
    }
}

void sig_handler(int signal) {
    switch(signal) {
        case SIGINT: //ignore sigint
            break;
        case SIGCHLD:
            while (waitpid((pid_t) (-1), 0, WNOHANG) > 0); //await children w/o hang (in case multiple finish)
            break;
        default:
            break;
    }
}

void command_controller(char * command);
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
        close_shell_err
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
                close_shell_err
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

size_t get_argc(char ** argv) {
    size_t argc = 0;
    char * arg;
    while ((arg = argv[argc]) != NULL) {
        argc++;
    }

    return argc;
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

void exec_command(char ** command_argv, int argc) {
    int background = argc >= 2 && !strcmp(command_argv[argc - 1], "&");
    background ||= command_argv[0][0] == '&';
    //delete the & character so exec doesn't treat it as an argument
    if (background) {
        command_argv[argc - 1] = NULL;
    }

    pid_t child_pid = fork();
    
    if (child_pid < 0) {
        //fork failed
        print_fork_failed();
        return;
    } else if (child_pid == 0) {
        //child process
        pid_t my_pid = getpid();

        if (background) {
            if (setpgid(my_pid, my_pid) == -1) {
                print_setpgid_failed();
                exit(EXIT_FAILURE);
            };
        }
        print_command_executed(my_pid); //command exec'd by <pid>
        execvp(command_argv[0], command_argv);
        print_exec_failed(command_argv[0]); //exec failed if this code is reached
        close_shell_err
    } else if (!background) {
         int status;
         waitpid(child_pid, &status, 0);
    }
    
}

void exec_cd(char ** command_argv, int argc) {
    if (argc < 2) {
        print_no_directory("");
    }

    char * path = command_argv[1];
    int chdir_result = chdir(path);
    if (chdir_result < 0) {
        print_no_directory(path);
    }
}

void list_history() {
    size_t idx = 0;
    for (; idx < vector_size(history); idx++) {
        char * history_line = vector_get(history, idx);
        print_history_line(idx, history_line);
    }
}

void search_by_index(const char * command) {
    if(strlen(command) < 2) {
        print_invalid_command(command);
        return;
    }

    size_t n = atoi(command + 1);

    if (n > vector_size(history) - 1) {
        print_invalid_index();
        return;
    }

    char * history_line = vector_get(history, n);
    print_command(history_line);
    command_controller(history_line);
}

void search_by_prefix(const char * command) {
    int prefix_length = strlen(command) - 1;
    if (prefix_length == 0) {
        search_by_index("#0");
    } else {
        size_t idx = 0;
        while (idx < vector_size(history)) {
            char * current_line = vector_get(history, idx);
            sstring * current_line_wrapper = cstr_to_sstring(current_line);
            char * current_prefix = sstring_slice(current_line_wrapper, 0, prefix_length);

            if (!strcmp(current_prefix, command + 1)) {
                print_command(current_line);
                command_controller(current_line);
                return;
            }

            idx++;
        }

        if (idx >= vector_size(history)) {
            print_no_history_match();
        }
    }
}

void command_controller(char * command) {
    char ** command_argv = parse_command(command);
    char * program_name = command_argv[0];
    int argc = get_argc(command_argv);

    int not_print = program_name[0] == '!' || program_name[0] == '#';
    if (!not_print) record_command(command);

    if (!strcmp(program_name, "cd")) {
        exec_cd(command_argv, argc);
    } else if (!strcmp(program_name, "!history")) {
        list_history();
    } else if (program_name[0] == '#') {
        search_by_index(program_name);
    } else if (program_name[0] == '!') {
        search_by_prefix(program_name);
    } else if (!strcmp(program_name, "exit")) {
        close_shell;
     } else {
        exec_command(command_argv, argc);
    }
    
}

void main_loop() {
    printf("in main loop\n");
    if (script_file_name) {
        script_file = fopen(script_fp, "r");
        if (!script_file) {
            print_script_file_error();
            close_shell_err
        }

    }
        printf("past script file check\n");
        FILE * command_src = script_file ? script_file : stdin;

        pid_t shell_pid  = getpid();
        char cwd[1024];
        char line[1024];
        while (1) {
            
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                print_prompt(cwd, shell_pid);
            }

            if (!fgets(line, sizeof(line), command_src)) {
                 close_shell
            }

            char * final_char = strchr(line, '\n');
            if (final_char) {
                *final_char = '\0';
            }

            
            if (command_src == script_file) {
                print_command(line);
            }

            command_controller(line);


        }

    fclose(script_file);
}
int shell(int argc, char *argv[]) {

    signal(SIGINT, sig_handler);
    signal(SIGCHLD, sig_handler);

    history = string_vector_create();
    parse_options(argc, argv);

    main_loop();

    return 0;
}
