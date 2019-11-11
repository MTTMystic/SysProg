/**
 * Finding Filesystems
 * CS 241 - Fall 2019
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    file_system * fs = open_fs("test.fs");
    void * buf = "This is a simple string. Let's see how things go.";
    off_t * offset = (off_t *) malloc(sizeof(off_t));
    *offset = 0;
    //test no offset
    int bytes_written = minixfs_write(fs, "./goodies/hello.txt", buf, sizeof(buf), offset);

    printf("bytes written ? : %d", bytes_written);
    //free(offset);
}
