/**
 * Nonstop Networking
 * CS 241 - Fall 2019
 */
#pragma once
#include <stddef.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
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

static const size_t buf_size = 1024;


typedef struct _req_header_t {
    verb method;
    char filename[255];
} req_header_t;

typedef struct _res_header_t {
    response status;
    char err_msg[1024];
} res_header_t;

req_header_t * string_to_req_header(char buffer[buf_size]);

verb string_to_verb(char * verb_str);

ssize_t get_filesize(int fd);








