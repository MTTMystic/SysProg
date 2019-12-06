/**
 * Nonstop Networking
 * CS 241 - Fall 2019
 */
#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>


#define LOG(...)                      \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
    } while (0);

typedef enum { GET, PUT, DELETE, LIST, V_UNKNOWN } verb;
typedef enum {OK, ERROR} response;
/**
 * PENDING = accepted connection but header not parsed
 * FILE_CHECK = header parsed + file check/setup (if applicable)
 * PARSE_FS = filename checked, parse expected filesize from client
 * DEL_FILE = delete file as per delete request
 * DATA_READ = read binary data from client
 * RES_HEADER = construct + send response header
 * SEND_FS = send filesize to client (if applicable)
 * DATA_WRITE = writing response data to client
 * ERR_REQ = invalid request (missing/malformed verb)
 * ERR_FS = amount of data received from client doesn't match expected filesize
 * ERR_NOFILE = received GET/DELETE request for file that does not exist
 * ERR = server encountered error not otherwise specified
 * CLOSED = client connection closed
 */
typedef enum {PENDING, 
              FILE_CHECK,
              PARSE_FS,
              DEL_FILE,
              DATA_READ,
              RES_HEADER,
              SEND_FS,
              DATA_WRITE,
              ERR_REQ,
              ERR_FS, 
              ERR_NOFILE,
              ERR, 
              CLOSED} status;

static const size_t buf_size = 1024;

ssize_t get_filesize(int fd);








