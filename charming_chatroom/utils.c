/**
 * Charming Chatroom
 * CS 241 - Fall 2019
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "utils.h"
static const size_t MESSAGE_SIZE_DIGITS = 4;

char *create_message(char *name, char *message) {
    int name_len = strlen(name);
    int msg_len = strlen(message);
    char *msg = calloc(1, msg_len + name_len + 4);
    sprintf(msg, "%s: %s", name, message);

    return msg;
}

ssize_t get_message_size(int socket) {
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}


// You may assume size won't be larger than a 4 byte integer
ssize_t write_message_size(size_t size, int socket) {
    int32_t message_size = htonl(size);
    ssize_t written_bytes = write_all_to_socket(socket, (char *)&message_size, MESSAGE_SIZE_DIGITS);
    if (written_bytes == 0 || written_bytes == -1) 
        return written_bytes;
    
    return (ssize_t)ntohl(size);
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    size_t bytes_read = 0;
    ssize_t read_result;
    while (bytes_read < count) {
        read_result = read(socket, buffer + bytes_read, count - bytes_read);
        
        if (read_result == 0) {
            return 0;
        }

        if (read_result == -1 && (errno == EINTR || errno == EAGAIN)) {
            errno = 0;
            continue;
        }

        if (read_result == -1) {
            return -1;
        }

        bytes_read += read_result;
    }

    return bytes_read;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    // Your Code Here
    ssize_t write_result = 0;
    size_t bytes_written = 0;

    //keep writing if there are more bytes to write and/or the write was interrupted
    while (bytes_written < count) {
        write_result = write(socket, buffer + bytes_written, count - bytes_written);
        
        if (write_result == 0) {
            return 0;
        }

        if (write_result == -1 && (errno == EINTR || errno == EAGAIN)) {
            errno = 0;
            continue;
        }

        if (write_result == -1) {
            return -1;
        }

        bytes_written += write_result;
    }
    
    return bytes_written;
}
