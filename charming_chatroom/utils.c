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
    ssize_t read_result = 0; //will track result of reading from socket
    ssize_t bytes_read = 0; //at start we read 0 bytes
    ssize_t remaining = count; //bytes remaining = count - read bytes
    
    //keep reading if there are more bytes to read and/or if the read was interrupted 
    char should_continue = (remaining > 0 && read_result != -1) || (read_result == -1 && errno == EINTR);
    while (should_continue) {
        read_result = read(socket, (void *) (buffer + bytes_read), remaining);
        //if we read 0 chars then there was nothing to read
        if (read_result == 0) {
            return 0;
        }

        //otherwise update bytes read and remaining
        //in the case that the read is interrupted, we may not read all bytes in one go
        if (read_result > 0) {
            bytes_read += read_result;
            remaining -= read_result;
        }

        should_continue = (remaining > 0 && read_result != -1) || (read_result == -1 && errno == EINTR);
    }

    return (read_result == -1) ? -1 : count;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    // Your Code Here
    ssize_t write_result = 0;
    ssize_t bytes_written = 0;
    ssize_t remaining = count;

    //keep writing if there are more bytes to write and/or the write was interrupted
    char should_continue = (remaining > 0 && write_result != -1) || (write_result == -1 && errno == EINTR);
    while (should_continue) {
        write_result = write(socket, (void *) (buffer + bytes_written), remaining);
        
        //if no bytes were written just return
        if (write_result == 0) {
            return 0;
        }

        if (write_result > 0) {
            bytes_written += write_result;
            remaining -= write_result;
        }

        should_continue = (remaining > 0 && write_result != -1) || (write_result == -1 && errno == EINTR);
    }

    return (write_result == -1) ? -1 : count;
}
