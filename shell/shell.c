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
#include <time.h>

typedef struct process {
    pid_t pid;
    long int nthreads;
    unsigned long int vsize;
    char state;
    char * start_str;
    char * time_str;
    char *command;
    clock_t exec_start;
} process;

/*
int pid;
    long int nthreads;
    unsigned long int vsize;
    char state;
    char *start_str;
    char *time_str;
    char *command;
*/
static vector * history = NULL;
static char * history_file_name = NULL;
static char * history_fp = NULL;
static FILE * history_file = NULL;

static char * script_file_name = NULL;
static char * script_fp = NULL;
static FILE * script_file = NULL;

static vector * processes = NULL;
#define close_shell_err cleanup(); exit(EXIT_FAILURE);
#define close_shell cleanup(); exit(EXIT_SUCCESS);


void cleanup() {
    while (waitpid((pid_t) (-1), 0, WNOHANG) > 0);
    
    vector_destroy(history);
    size_t idx = 0;
    
    if (processes) {
        for (; idx < vector_size(processes); idx++) {
            process * process = vector_get(processes, idx);
            if (process) {
                free(process->start_str);
                free(process->time_str);
                free(process);
            } 
        }
        
        vector_destroy(processes);
    }
    
    
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
    int print = command[0] != '!' && command[0] != '#' && strcmp(command, "exit");
    if (!print) return;
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

    size_t bytes_needed = sizeof(char *) * (arg_count + 1);

    char ** command_args = malloc(bytes_needed);  
    
    size_t arg_idx = 0;
    for (; arg_idx < arg_count; arg_idx++) {
        char * arg = vector_get(args, arg_idx);
        if (arg) {
             command_args[arg_idx] = strdup(arg);
        }
    }

    command_args[arg_count] = NULL; 

    sstring_destroy(command_wrapper);
    vector_destroy(args);
    return command_args;
}

void free_command_args(char ** argv) {
    if (!argv) return;
    size_t idx = 0;
    size_t argc = get_argc(argv);
    for(; idx < argc; idx++) {
        if (!argv[idx]) continue;
        free(argv[idx]);
        argv[idx] = NULL;
    }

    free(argv);
    argv = NULL;
}


int logical_operator_idx(char ** command_argv) {
    size_t idx = 0;
    for (; idx < get_argc(command_argv); idx++) {
        char * current = command_argv[idx];
        int operator = !strcmp(current, "&&") || !strcmp(current, "||") || !strcmp(current, ";");
        if (operator) {
            return idx;
        }
    }

    return -1;
}


void get_proc_info(process * proc) {
    char file_name_buffer[50];
    sprintf(file_name_buffer, "/proc/%d/status", proc->pid);
    FILE * proc_file = fopen(file_name_buffer, "r");

    char line[1024];
    while(fgets(line, 1024, proc_file)) {
        if (!strncmp(line, "State:", 6)) {
            proc->state = line[7];
        } else if (!strncmp(line, "VmSize", 6)) {
            char vsize_string[16];
            sscanf(line + 7, "%s", vsize_string);
            proc->vsize = atoi(vsize_string);
        } else if (!strncmp(line, "Threads:", 8 )) {
            char no_threads[16];
            sscanf(line + 8, "%s", no_threads);
            proc->nthreads = atoi(no_threads);
        }

        double exec_time = (double)(clock() - proc->exec_start) / CLOCKS_PER_SEC;
        int minutes = exec_time / 60;
        int secs = (int) (exec_time - (minutes * 60));
        char time_buffer[16];
        sprintf(time_buffer, "%d:%0.2d", minutes, secs);
        proc->time_str = strdup(time_buffer);
    }

    fclose(proc_file);
}

process_info * make_proc_info(process * proc) {
    process_info * proc_info = (process_info *) malloc(sizeof(proc_info));
    proc_info->pid = proc->pid;
    proc_info->nthreads = proc->nthreads;
    proc_info->vsize = proc->vsize;
    proc_info->state = proc->state;
    proc_info->start_str = proc->start_str;
    proc_info->time_str = proc->time_str;
    proc_info->command = proc->command;
    return proc_info; 
}

void exec_pfd(pid_t pid) {
    print_process_fd_info_header();
    
    char file_name_buffer[50];
    sprintf(file_name_buffer, "/proc/%d/fdinfo/%d", pid, 0);
   
}

void exec_ps() {
    print_process_info_header();
    size_t idx = 0;
    for (; idx < vector_size(processes); idx++) {
        process * proc = vector_get(processes, idx);
        if (proc) {
            get_proc_info(proc);
            if (proc->state == 'R' || proc->state == 'S') {
                process_info * proc_info = make_proc_info(proc);
                print_process_info(proc_info);
                free(proc_info);
            }
        }
       
    }
}

char * get_original_command(int argc, char ** argv) {
    printf("in goc\n");
    size_t bytes_needed = 0;
    int idx = 0;
    for(; idx < argc; idx++) {
        if (idx < argc - 1) bytes_needed++;
        bytes_needed += strlen(argv[idx]);
    }
    printf("out of first loop \n");
    bytes_needed++;
    char * original_command = calloc(bytes_needed, bytes_needed);
    original_command[strlen(original_command) + 1] = '\0';

    idx = 0;
    for(; idx < argc; idx++) {
        original_command = strcat(original_command, argv[idx]);
        if (idx < argc - 1) original_command[strlen(original_command)] = ' ';
    }
   
    return original_command;
}

void track_process(char ** argv, pid_t pid) {
    process * process_obj = calloc(sizeof(process), 1);
    time_t start = time(NULL);
    process_obj->pid = pid;
    if (!strcmp(argv[0], "./shell")) {
        process_obj->command = "./shell";
    } else {
        process_obj->command = get_original_command(get_argc(argv), argv);
    }

    struct tm * start_time = localtime(&start);
    char time_buffer[256];
    strftime(time_buffer, sizeof(time_buffer), "%H:%M", start_time);
    process_obj->start_str = strdup(time_buffer);

    //printf("%s\n", process_obj->start_str);
    process_obj->exec_start = clock();
    vector_push_back(processes, process_obj);

    //get_proc_info(process_obj);
}

/**
 * Execute the given command.
 * TODO: add toggle between execution of built-in commands and external commands
 * TODO: add support for backgrounding
 */ 

void exec_command_helper(char ** command_argv, int argc, int * status_addr) {
    int background_arg = argc >= 2 && !strcmp(command_argv[argc-1], "&");
    char * last_arg = command_argv[argc - 1];
    int background_char = (!background_arg) && last_arg[strlen(last_arg) - 1] == '&';

    //delete the & character so exec doesn't treat it as an argument
    if (background_arg) {
        command_argv[argc - 1] = NULL;
    } else if (background_char) {
        last_arg[strlen(last_arg) - 1] = '\0';
    }

    int background = background_arg || background_char;
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
            track_process(command_argv, my_pid);
        }

        print_command_executed(my_pid); //command exec'd by <pid>
        execvp(command_argv[0], command_argv);
        print_exec_failed(command_argv[0]); //exec failed if this code is reached
        free_command_args(command_argv);
        close_shell_err

    } else if (!background) {
         int child_status;
         int * status_arg = status_addr ? status_addr : &child_status;
         waitpid(child_pid, status_arg, 0);
    }
    
}

void exec_command(char ** command_argv, int argc) {
    exec_command_helper(command_argv, argc, NULL);
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

void logical_operator_helper(char ** command_argv, int * status_addr) {
    if (!command_argv) {
       *status_addr = EXIT_FAILURE;
       return; 
    }

    char * program_name = command_argv[0];
    int argc = get_argc(command_argv);

    //printf("program name is %s\n", program_name);
    if (!strcmp(program_name, "cd")) {
        exec_cd(command_argv, argc);
    } else {
        exec_command_helper(command_argv, argc, status_addr);
    }
}

/**Execute the && operator on two given commands.
 * Behavior: run the first command. If it exited successfully, run the second command.
 * Otherwise, short-circuit
 */ 
void exec_and(char ** first_command_argv, char ** second_command_argv) {
    int status = -1;
    logical_operator_helper(first_command_argv, &status);
    int short_circuit = (status != -1) && WIFEXITED(status) && WEXITSTATUS(status);
    if (!short_circuit) {
         logical_operator_helper(second_command_argv, &status);
    }
}

void exec_or(char ** first_command_argv, char ** second_command_argv) {
    int status = -1;
    logical_operator_helper(first_command_argv, &status);
    int short_circuit = (status != -1) && WIFEXITED(status) && WEXITSTATUS(status);
    if (short_circuit) {
         logical_operator_helper(second_command_argv, &status);
    }
}

void exec_separated(char ** first_command_argv, char ** second_command_argv) {
    logical_operator_helper(first_command_argv, NULL);
    logical_operator_helper(second_command_argv, NULL);
}

void logical_operator_controller(char ** command_argv, int operator_idx) {
    int argc = get_argc(command_argv);
    //remember that extra NULL element terminates argv for shell commands
    size_t bytes_needed = sizeof(char *) * (operator_idx + 1); //bytes needed for first command

    char ** first_command_argv = calloc(bytes_needed, (operator_idx + 1)); //zero-out memory

    bytes_needed = sizeof(char * ) * (argc - operator_idx); //
    char ** second_command_argv = calloc(bytes_needed, argc - operator_idx); //zero-out memory

    int idx = 0;
    for (; idx < operator_idx; idx++) {
        first_command_argv[idx] = command_argv[idx];
    }

    int offset = operator_idx + 1;
    for (idx = offset; idx < (int) get_argc(command_argv); idx++) {
        second_command_argv[idx - offset] = command_argv[idx];
        //printf("%s\n", second_command_argv[idx - offset]);
    }

    char * operator = command_argv[operator_idx];
    if (!strcmp(operator, "&&")) {
        //printf("executing and\n");
        exec_and(first_command_argv, second_command_argv);
    } else if (!strcmp(operator, "||")) {
        exec_or(first_command_argv, second_command_argv);
    } else if (!strcmp(operator, ";")) {
        exec_separated(first_command_argv, second_command_argv);
    }

    free(first_command_argv);
    first_command_argv = NULL;
    free(second_command_argv);
    second_command_argv = NULL;
}

void command_controller(char * command) {
  
    char ** command_argv = parse_command(command);
    char * program_name = command_argv[0];

    int argc = get_argc(command_argv);
    
    record_command(command);

    int operator_idx = logical_operator_idx(command_argv);
    if (operator_idx != -1) {
        //printf("branch logic controller\n");
        logical_operator_controller(command_argv, operator_idx);
    } else if (!strcmp(program_name, "cd")) {
        //printf("exec cd\n");
        exec_cd(command_argv, argc);
    } else if (!strcmp(program_name, "!history")) {
        list_history();
    } else if (program_name[0] == '#') {
        search_by_index(program_name);
    } else if (program_name[0] == '!') {
        search_by_prefix(program_name);
    } else if (!strcmp(program_name, "exit")) {
        free_command_args(command_argv);
        close_shell;
    } else if (!strcmp(program_name, "ps")) {
        exec_ps();
    } else if (!strcmp(program_name, "pfd")) {
        exec_pfd((pid_t)(atoi(command_argv[1])));  
    } else {
        exec_command(command_argv, argc);
    }
    
    free_command_args(command_argv);
}


void main_loop() {
    if (script_file_name) {
        script_file = fopen(script_fp, "r");
        if (!script_file) {
            print_script_file_error();
            close_shell_err
        }

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
    
    processes = shallow_vector_create();
    track_process(argv, getpid());
    main_loop();

    return 0;
}
