/**
 * Nonstop Networking
 * CS 241 - Fall 2019
 */
#include "common.h"
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct _args_t {
    char * host;
    char * port;
    char * method;
    char * remote;
    char * local;
} args_t;


static args_t args_o;
static int serverSocket;
static int local_file_fd;
static char ** args;
static char * request_header;
static char * response_header;


char **parse_args(int argc, char **argv);
verb check_args(char **args);

/**Constructs args_t struct from args char ** */
void get_args_t(char ** args);
/**Cleans up heap memory on exit.*/
void cleanup();

/**Opens file for writing for GET requests. 
 * If the file does not exist, creates it with full (rwx) permissions.
*/
void setup_get_file();

/**Opens file for reading for PUT requests.
 * Will exit client if local file does not exist.*/
void setup_put_file();

void put_request();

void get_request();

/**Establishes connection to server.*/
void connect_to_server();

/**Writes buffer of [size] to server.
 * Retries on write interrupt.
 * Exits when encountering closed/disconnected server.
 * Blocking.
 * 
 * Returns number of bytes written or -1 on failure.
*/
ssize_t write_to_server(char * buffer, size_t size);

/**
 * Reads [size - 1] bytes from server and stores them in provided buffer.
 * Will automatically add null-terminating byte to end of buffer
 * Retries on read interrupt.
 * Exits when encountering closed server.
 * Blocking.
 * 
 * Returns bytes written.
 * 
 */ 
ssize_t read_from_server(char * buffer, size_t size);

/**Fills buffer of [size] by reading chars from local file.
 * Only useful for PUT requests.
 * Returns number of bytes written to buffer.*/
int fill_buffer_from_file(char * buffer, size_t size);

/**Forms header string to send to server.*/
void req_header_string();

void res_header_string();

void fix_split_buffer(char * split, char * new_buf, size_t split_idx);

int main(int argc, char **argv) {
    // Good luck!
    
    args = parse_args(argc, argv);
    LOG("parsed args");
    verb method = check_args(args);
    LOG("checked args");

    get_args_t(args);
    req_header_string();
    connect_to_server();
    write_to_server(request_header, strlen(request_header));

    if (method == GET) {
        //setup local file for writing
        setup_get_file();
    } else if (method == PUT) {
        //setup local file, get + send size, send binary data
        put_request();
    }

    //alert the server that we are done writing
    shutdown(serverSocket, SHUT_WR);
    //start reading response from server
    char response_buffer[buf_size + 1];
    read_from_server(response_buffer, buf_size + 1);
    //try to parse header
    res_header_string(response_buffer);
    cleanup();
}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}

/**Takes null-terminated array in form {host, port, method, remote, local, NULL} and makes args_t struct.*/


void get_args_t(char ** args) {
    if (args == NULL) {
        LOG("didn't receive valid args input");
        return;
    }

    args_o.host = args[0];
    args_o.port = args[1];
    args_o.method = args[2];
    args_o.remote = args[3];
    args_o.local = args[4];
    
    LOG("successfully made args struct");

};

void cleanup() {
    free(args);
    free(request_header);
}

void setup_get_file() {
    //if the file exists, truncate the file
    //if it does not, create the file
    //create file with full permissions
    local_file_fd = open(args_o.local, O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG |S_IRWXO);
    if (local_file_fd >= 0) {
        LOG("sucessfully opened the file");
    } else {
        LOG("could not open the file");
    }

    LOG("fd is %d", local_file_fd);
    
}

void setup_put_file() {
    char file_exists = access(args_o.local, R_OK) != -1;
    if (!file_exists) {
        LOG("local file does not exist");
        cleanup();
        exit(1);
    }

    local_file_fd = open(args_o.local, O_RDONLY);
    LOG("opened file for reading");
}

void put_request() {
    setup_put_file();
    ssize_t filesize = get_filesize(local_file_fd);
    write_to_server((char *) &filesize, 8); //send filesize to server

    size_t bytes_written = 0;
    char buffer[buf_size];
    //send file to server
    //handling large files: read and send only 1204 bytes at a time
    while (bytes_written < (size_t) filesize) {
        size_t buffer_len = fill_buffer_from_file(buffer, buf_size);
        bytes_written += write_to_server(buffer, buffer_len);
    }
    
}

void connect_to_server() {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(args_o.host, args_o.port, &hints, &result);

    if (ret != 0) {
        perror("failed to get address info: ");
        freeaddrinfo(result);
        cleanup();
        exit(1);
    }

     //-----------establish connection--------

    LOG("attempting to connect....");
     //setup socket to be reusable
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int));
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
    //try to connect
    int connect_ret = connect(serverSocket, result->ai_addr, result->ai_addrlen);
    if (connect_ret == -1) {
        perror("failed to establish connection: ");
        freeaddrinfo(result);
        exit(1);
    }
    LOG("successfully connected to the server");
    freeaddrinfo(result);
}

void req_header_string() {
    request_header = malloc(buf_size + 1);
    if (!request_header) {
        LOG("failed to allocate memory for request header");
    }
    
    size_t header_len = 0;
    for (size_t idx = 0; idx < strlen(args_o.method); idx++) {
        request_header[idx] = args_o.method[idx];
    }

    header_len += strlen(args_o.method);

    //space after VERB only needed for requests other than LIST
    if (strcmp(args_o.method, "LIST")) {
        request_header[header_len] = ' ';
        header_len++;
    }
    
    //add name of remote file if it is applicable
    if (args_o.remote) {
        for (size_t idx = 0; idx < strlen(args_o.remote); idx++) {
          request_header[header_len + idx] = args_o.remote[idx];
        }

        header_len += strlen(args_o.remote);

    }
    
    //add newline character
    request_header[header_len] = '\n';
    header_len++;
    //null terminator for safety
    request_header[header_len] = 0;

    //shrink header
    request_header = realloc(request_header, header_len+1);
    LOG("the request header so far is: %s", request_header);
}

ssize_t write_to_server(char * buffer, size_t size) {
    int retval;
    size_t bytes_written = 0;
    while (bytes_written < size) {
        retval = write(serverSocket, buffer + bytes_written, size - bytes_written);
        if (retval == 0) {
            LOG("server closed");
            break;
        }

        if (retval == -1 && errno == EINTR) {
            LOG("write was interrupted");
            errno = 0;
            continue;
        }

        if (retval == -1) {
            perror("write failed: ");
            cleanup();
            exit(1);
        }

        bytes_written += retval;
        LOG("sent %d bytes to server", retval);
    }

    return bytes_written;
}

int fill_buffer_from_file(char * buffer, size_t size) {
    int read_result = read(local_file_fd, buffer, size);
    if (read_result == -1) {
        perror("failed to read from file: ");
    }

    LOG("read %d bytes from file", read_result);
    buffer[read_result] = 0;
    return read_result;
}

ssize_t read_from_server(char * buffer, size_t size) {
    size_t bytes_read = 0;
    int retval = 0;
    size_t target = size - 1;
    while (bytes_read < target) {
        retval = read(serverSocket, buffer + bytes_read, target - bytes_read);
        if (retval == 0) {
            LOG("server closed");
            break;
        }

        if (retval == -1 && errno == EINTR) {
            LOG("read was interrupted");
            errno = 0;
            continue;
        }

        if (retval == -1) {
            perror("read failed :");
            cleanup();
            exit(1);
           
        }
        LOG("recieved %d bytes from server", retval);
        bytes_read += retval;
    }

    buffer[bytes_read] = 0;
    return bytes_read;
}

void res_header_string(char * buffer) {
    size_t status_len = strcspn(buffer, "\n");
    size_t str_len = strlen(buffer);
    size_t header_len = 0;

    if (status_len == str_len) {
        print_invalid_response();
    }

    response_header = malloc(buf_size + 1);
    strncpy(response_header, buffer, status_len + 1);
    header_len += status_len + 1;

    

    if (!strcmp(response_header, "ERROR\n")) {
        size_t error_len = strcspn(buffer + header_len, "\n");
        if (error_len == str_len) {
            print_invalid_response();
        }
        strncpy(response_header + header_len, buffer + header_len, error_len + 1);
        header_len += error_len + 1;
    }

    response_header[header_len] = 0;
    
    LOG("parsed response header is: %s", response_header);
}