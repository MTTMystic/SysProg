/**
 * Nonstop Networking
 * CS 241 - Fall 2019
 */
#include "common.h"

ssize_t get_filesize(int fd) {
    if (fd < 0) {
        LOG("no such file exists");
        return -1;
    }

    struct stat f_stat;
    fstat(fd, &f_stat);
    return f_stat.st_size;
    
}