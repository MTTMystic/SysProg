/**
 * Password Cracker
 * CS 241 - Fall 2019
 */
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "includes/queue.h"

#include <unistd.h>
#include <crypt.h>
#include <string.h>
#include <stdio.h>


static struct crypt_data cdata;
static struct queue * task_queue;

typedef struct _task {
    char username[9]; //maximum 8 chars + null terminator
    char pw_hash[14]; //maximum 13 chars + null terminator
    char known_chars[9]; //same length rule as u/n
} task;

/**
 * Converts given line of input to a task struct, and returns pointed to alloc'd task
 * 
 * @param input_line
 * A line which contains space-separated values corresponding to task properties
 * 
 * @return
 * Pointer to task struct matching given input
 * NULL if input is empty
 */ 
task * string_to_task(char * input_line) {
    //return NULL ptr if given input is empty string
    if (strlen(input_line) == 0) {
        return NULL;
    }

    //malloc new memory for task struct
    task * new_task = (task *) malloc(sizeof(task));
    //place space-separated strings into vars
    sscanf(input_line, "%s %s %s", new_task->username, new_task->pw_hash, new_task->known_chars);
    return new_task;
};

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads

    //set val for initialized before using cdata
    cdata.initialized = 0;
    //create task queue w/o maximum size
    task_queue = queue_create(0);
    task * example = string_to_task("donna xxC4UjY9eNri6 hello...");

    printf("%s %s %s\n", example->username, example->pw_hash, example->known_chars);
    //create [thread_count] threads to start in crack_password

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
