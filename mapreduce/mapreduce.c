/**
 * mapreduce
 * CS 241 - Fall 2019
 */
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

/**
 * Simple helper function to create a splitter process.
 * @param input_file
 *  name of file to split
 * @param num_chunks
 * # of separate chunks to split input_file into
 * @param chunk_idx
 * # of the current chunk for the splitter (zero-indexed)
 * @param out_pipe
 * where the output should be directed (will need to replace stdout with outpipe)
 */ 

void splitter_exec(char * input_file, int num_chunks, int chunk_idx, int out_pipe) {
    //create child process for splitter
    pid_t fork_id = fork();

    if (fork_id == -1) {
        exit(1);
    }

    //subprocess
    if (fork_id == 0) {
        int replace_result = dup2(out_pipe, 1); //try to replace stdout with given output fd
        if (replace_result == -1) {
            exit(1);
        }

        //close + destroy duplicated fds
        //after replacing stdout with out_pipe, no need for other fds
        descriptors_closeall();
        descriptors_destroy();

        //convert int args to char
        char num_chunks_str[32];
        char chunk_idx_str[32];

        sprintf(num_chunks_str, "%d", num_chunks);
        sprintf(chunk_idx_str, "%d", chunk_idx);       

        //exec splitter
        execlp("./splitter", "./splitter", input_file, num_chunks_str, chunk_idx_str, NULL);
        exit(1);
    }   
}

/**
 * Same purpose as splitter_exec above but generalized for mapper + reducer processes
 * @param exe_name
 * Name of executable to run in subprocess
 * @param in_pipe
 * FD to use for replacing stdin (-1 if replacement not needed)
 * @param out_pipe
 * FD to use for replacing stdout (-1 if replacement not needed)
 * @param fork_id
 * Addr to use for storing fork result id (for caller waiting)
 */ 
void exec_helper(char * exe_name, int in_pipe, int out_pipe, int * fork_id) {
    *fork_id = fork();

    //fork failed
    if (*fork_id == -1) {
        exit(1);
    }

    //subprocess
    if (*fork_id == 0) {
        if (in_pipe != -1) {
            int replace_in_result = dup2(in_pipe, 0);
            if (replace_in_result == -1) {
                exit(1); //replace failed
            }
        }

        if (out_pipe != -1) {
            int replace_out_result = dup2(out_pipe, 1);
            if (replace_out_result == -1) {
                exit(1); //replace failed
            }
        }

        //like with splitter_exec, no need for additional fds after replacement
        descriptors_closeall();
        descriptors_destroy();


        execlp(exe_name, exe_name, NULL);
        exit(1);


    }
}

int main(int argc, char **argv) {
    if (argc < 6) {
        print_usage();
        exit(1);
    }

    char * input_file_name = argv[1];
    //printf("%s\n", input_file_name);
    char * output_file_name = argv[2];
    //printf("%s\n", output_file_name);
    char * mapper_exe = argv[3];
    //printf("%s\n",mapper_exe);
    char * reducer_exe = argv[4];
    //printf("%s\n",reducer_exe);
    char * n_mappers_str = argv[5];
    

    int n_mappers = atoi(n_mappers_str);
    printf("%d\n", n_mappers);

    // Create an input pipe for each mapper.
    int pipes_size = 2 * n_mappers * sizeof(int); //need 2 fds (ints) per pipe per mapper
    int * mapper_pipes = malloc(pipes_size);

    int idx = 0;
    //create pipes
    for (; idx < n_mappers; idx++) {
        int pipe_idx = idx * 2; //read end of pipe always at even indices
        int pipe_result = pipe2(mapper_pipes + pipe_idx, O_CLOEXEC);
        if (pipe_result == -1) {
            exit(1);
        }

        //track newly created pipes as fds
        descriptors_add(mapper_pipes[pipe_idx]);
        descriptors_add(mapper_pipes[pipe_idx + 1]);
    }

    // Create one input pipe for the reducer.
    int reducer_pipe[2]; 
    int reducer_pipe_result = pipe2(reducer_pipe, O_CLOEXEC);
    if (reducer_pipe_result == -1) {
        exit(1);
    }

    //track reducer pipe as fd
    descriptors_add(reducer_pipe[0]);
    descriptors_add(reducer_pipe[1]);

    // Open the output file.
    int out_fd = open(output_file_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (out_fd == -1) {
        exit(1);
    }

    descriptors_add(out_fd);

    // Start a splitter process for each mapper.
    idx = 0;
    for (; idx < n_mappers; idx++) {
        splitter_exec(input_file_name, n_mappers, idx, mapper_pipes[(idx * 2) + 1]); //direct splitter output to write end
    }
    // Start all the mapper processes.
    pid_t * mapper_ids = malloc(sizeof(pid_t) * n_mappers); //track pids for each map subproc
    idx = 0;
    for (; idx < n_mappers; idx++) {
        //mark input as read end of mapper pipe
        //mark output as write end of reducer pipe
        exec_helper(mapper_exe, mapper_pipes[idx * 2], reducer_pipe[1], &mapper_ids[idx]);
    }

    // Start the reducer process.
    pid_t reducer_id;
    //mark input as read end of reducer pipe
    //mark output as fd for given output file
    exec_helper(reducer_exe, reducer_pipe[0], out_fd, &reducer_id);
    // Wait for the reducer to finish.
    int status;
    int wait_result = waitpid(reducer_id, &status, 0);
    if (wait_result == -1) {
        exit(1);
    }
    // Print nonzero subprocess exit codes.

    // Count the number of lines in the output file.

    return 0;
}
