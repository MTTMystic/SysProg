/**
 * Utilities Unleashed
 * CS 241 - Fall 2019
 */

#include "format.h"
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    //check arguments
    //argc >= 2 because command must specify program to exec
    int sufficient_args = argc > 1;
    if (!sufficient_args) {
        print_time_usage();
    }

    //for marking process begin & process end time
    struct timespec begin, finish;
    
    
    int child_status;
    clock_gettime(CLOCK_MONOTONIC, &begin); //mark beginning of attempt

    pid_t fork_id = fork();

    if (fork_id < 0) {
        //fork failed
        print_fork_failed();
        exit(1);
    } else if (fork_id == 0) {
        //in child process
        
        execvp(argv[1], argv+1); //skip past 1st elem in argv array (time program name)
        print_exec_failed(); //print + exit never reached unless exec failed
        exit(1);
    } else {
        waitpid(fork_id, &child_status, 0); //wait for child to finish   
    }

    clock_gettime(CLOCK_MONOTONIC, &finish); //finished trying to exec program
    if (WIFEXITED(child_status) && WEXITSTATUS(child_status)) {
        exit(1); //don't bother printing results if child didn't exit successfully
    }

    double elapsed_time_sec = (double)(finish.tv_sec - begin.tv_sec);
    double elapsed_time_nsec = (double)(finish.tv_nsec - begin.tv_nsec); //calculate fractional seconds

    int nanos_per_sec = 100000000;

    double elapsed_time = elapsed_time_sec + (elapsed_time_nsec / nanos_per_sec);
    display_results(argv, elapsed_time);
    return 0;
}
