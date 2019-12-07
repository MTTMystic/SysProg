/**
 * Nonstop Networking
 * CS 241 - Fall 2019
 */
#include "common.h"
#include "format.h"
#include "vector.h"
#include "dictionary.h"
#include <signal.h>
#include <sys/epoll.h>
#include <math.h>

typedef struct _connection_t {
    /**fd of client socket*/
    int fd;
    /**status of connection*/
    status stat;
    /**requested VERB, applicable only if status != PENDING*/
    verb method;
    /**specified filename on server*/
    char filename[255];
    /**[only applies for PUT requests] amt of binary data to be transferred*/
    size_t file_size;

    /**buffer for storing header info, including size*/
    char header[buf_size + 1];
    /**how many bytes are in the header*/
    size_t header_bytes;

    char fs_buf[8];

    bool read_fs;

    /**buffer for storing read/write data for files*/
    char buf[buf_size + 1];
    /**stores buffer offset*/
    size_t buf_offset;
    /**bytes written to client*/
    size_t bytes_written;
    /**bytes read from client*/
    size_t bytes_read;

    size_t file_offset;

    bool closed_read;
    bool closed_write;
} connection_t;


static char * temp_dir = NULL; //folder name of temp dir
static vector * file_list; //global list of files
static vector * connections; //global list of client info

static bool endSession; //flag to exit server automata
static volatile int serverSocket; //socket for incoming connections

static volatile int epoll_fd;
static int clientCount;

void close_connection(connection_t * client_data);
int del_file(char * filename);
/**--------------------------HOUSEKEEPING & UTILITY------------------------*/

void cleanup() {
    if (file_list && !vector_empty(file_list)) {
        vector_destroy(file_list);
    }

    if (connections) {
       for (int i = vector_size(connections) - 1; i >= 0; i--) {
           if (vector_get(connections, i)) {
               free(vector_get(connections, i));
           }
       }
    }

    if (temp_dir) {
        for (size_t i = 0; i < vector_size(file_list); i++) {
            char * filename = vector_get(file_list, i);
            del_file(filename);
        }

        del_file(temp_dir);
    }

    close(epoll_fd);
}

void close_server(int signal) {
    if (signal == SIGINT) {
        cleanup();
        endSession = true;
    }
}

void exit_fail() {
    close_server(SIGINT);
    exit(1);
}

verb string_to_verb(char * m) {
    if (!strcmp(m, "GET")) {
        LOG("detected GET request");
        return GET;
    } else if (!strcmp(m, "PUT")) {
        LOG("detected PUT request");
        return PUT;
    } else if (!strcmp(m, "LIST\n")) {
        LOG("detected LIST request");
        return LIST;
    } else if (!strcmp(m, "DELETE")) {
        LOG("detected DELETE request");
        return DELETE;
    }

    return V_UNKNOWN;

}

int del_file(char * filename) {
    int retval = remove(filename);
    if (retval == -1) {
        perror("remove(): ");
    }
    return retval;
}

/**-------------------------SOCKET MANAGEMENT-----------------*/

void setup_server_socket(char * port) {
    if (!port) {
        LOG("No port specified. Cannot bind server socket.");
        exit_fail();
    }

    //get addrinfo
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int retval = getaddrinfo(NULL, port, &hints, &result);

    if (retval != 0) {
        LOG("Failed on getaddrinfo: %d", retval);
        exit_fail();
    }
    
    //setup reusable, non-blocking socket
    serverSocket = socket(AF_INET, SOCK_STREAM ,0);
    
    int optval = 1;
    retval = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    retval = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    if (retval == -1) {
        perror("setsockopt() : ");
        exit_fail();
    }

    
    //make listening socket non-blocking
    retval = fcntl(serverSocket, F_SETFL, O_NONBLOCK);
    if (retval == -1) {
        perror("set nonblocking: ");
        exit_fail();
    }
    
   
    //bind socket
    retval = bind(serverSocket, result->ai_addr, result->ai_addrlen);
    if (retval == -1) {
        perror("bind(): ");
        exit_fail();
    }

    retval = listen(serverSocket, 10);
    if (retval == -1) {
        perror("listen() :");
        exit_fail();
    }

    LOG("successfully established server socket on port %s", port);

}

connection_t * connection_setup(int fd) {
    connection_t * new_conn = (connection_t *) malloc(sizeof(connection_t));
    if (!new_conn) {
        perror("Failed to allocate memory for new connection.");
        exit_fail();
    }

    new_conn->fd = fd;
    new_conn->stat = PENDING;

    memset(new_conn->buf, 0, sizeof(new_conn->buf));
    memset(new_conn->header, 0, sizeof(new_conn->header));
    memset(new_conn->fs_buf, 0, sizeof(new_conn->fs_buf));
    memset(new_conn->filename, 0, sizeof(new_conn->filename));

    new_conn->file_size = pow(2, 8) - 1;
    new_conn->header_bytes = 0;
    new_conn->buf_offset = 0;
    new_conn->closed_read = false;
    new_conn->closed_write = false;
    new_conn->file_offset = 0;
    new_conn->read_fs = false;
    new_conn->bytes_read = 0;
    new_conn->bytes_written = 0;

    vector_push_back(connections, new_conn);

    return new_conn;
}

void epoll_event(int fd, void * data_ptr, uint32_t control_type, uint32_t flags) {
    struct epoll_event ev;
    ev.events = flags;
    ev.data.fd = fd;
    ev.data.ptr = data_ptr;

    int ret = epoll_ctl(epoll_fd, control_type, fd, &ev);
    
    if (ret == -1) {
        perror("epoll_ctl() : ");
        exit_fail();
    }
}

/**do-while loop to accept incoming connections & associated setup*/
void accept_connections() {
   int new_cli_fd = -1;
   do {
       struct sockaddr_in client_addr;
       socklen_t client_addr_len = sizeof(client_addr);
       new_cli_fd = accept(serverSocket, &client_addr, &client_addr_len);
       //setup connection data for client
       if (new_cli_fd != -1) {
            LOG("Connected to new client. Fd is %d", new_cli_fd);
            connection_t * new_conn = connection_setup(new_cli_fd);
            clientCount++;
            epoll_event(new_cli_fd, (void*)new_conn, EPOLL_CTL_ADD, EPOLLIN);
        }
      
   } while (new_cli_fd != -1);
}


void close_connection(connection_t * client_data) {
    LOG("closing connection??");
    for (size_t i = 0; i < vector_size(connections); i++) {
        if (vector_get(connections, i) == client_data) {
            vector_erase(connections, i);
            break;
        }
    }
    //stop listening for events
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_data->fd, NULL);
    //shutdown client socket
    shutdown(client_data->fd, SHUT_RDWR);
    //close socket
    close(client_data->fd);
    //free
    if (client_data) {
        free(client_data);
    }
    
}
/**------------------SOCKET READS/WRITES------------*/
/*attempts to read up to [n_bytes] bytes from socket into given buffer
 ASSUMES that buf has enough room for n_bytes + 1 (null terminator)*/
int read_from_socket(connection_t * client_data, char * buf, size_t n_bytes) {
    size_t bytes_read = 0;

    bool no_data_left = false;

    while (bytes_read < n_bytes) {
        int retval = read(client_data->fd, buf + bytes_read, n_bytes - bytes_read);
        
        if (retval == 0) {
            LOG("connection closed on %d", client_data->fd);
            client_data->closed_read = true;    
            break;
        }

        if (retval == -1) {
            switch(errno) {
                case EINTR:
                    continue;
                case EAGAIN | EWOULDBLOCK:
                    no_data_left = true;
                default:
                    perror("read(): ");
                    break;
            }
        }
  
        
    
        bytes_read += retval;
    }

    LOG("read %zu bytes from %d", bytes_read, client_data->fd);
    client_data->bytes_read += bytes_read;
    if (buf == client_data->buf) {
        client_data->buf_offset += bytes_read;
    }
    
    
    if (no_data_left) {
        return -1;
    }

    return bytes_read;
}

void read_header(connection_t * client_data) {
    char * header = client_data->header + client_data->header_bytes;
    int remaining_bytes = sizeof(client_data->header) - client_data->header_bytes - 1;
    //LOG("bytes remaining in header: %d", remaining_bytes);
    read_from_socket(client_data, header, remaining_bytes);
    //if the client disconnects before header has been sent and parsed, send request error
    if (client_data->closed_read) {
        client_data->stat = ERR_REQ;
    }
    client_data->header_bytes = client_data->bytes_read;
}

bool read_filesize(connection_t * client_data) {
    size_t bytes_read = 0;
    LOG("parsing filesize");
    while (bytes_read < sizeof(size_t)) {
        bytes_read = read_from_socket(client_data, client_data->fs_buf, 8 - bytes_read);
        
        //handle case where client disconnected early
        if (client_data->closed_read) {
            client_data->stat = CLOSED;
            return false;
        }
    }
    
    client_data->read_fs = true;
    return true;
    
}

int write_to_socket(connection_t * client_data, char * buf, size_t n_bytes) {
    size_t bytes_written = 0;

    bool no_room_left = false;

    while (bytes_written < n_bytes) {
        int retval = write(client_data->fd, buf + bytes_written, n_bytes - bytes_written);
        
        if (retval == 0) {
            LOG("connection closed on %d", client_data->fd);
            client_data->closed_write = true;    
            break;
        }

        if (retval == -1) {
            switch(errno) {
                case EINTR:
                    continue;
                case EAGAIN | EWOULDBLOCK:
                    no_room_left = true;
                default:
                    perror("write(): ");
                    break;
            }
        }
  
        bytes_written += retval;
    }

    LOG("wrote %zu bytes to %d", bytes_written, client_data->fd);
    client_data->bytes_written += bytes_written;
    
    if (no_room_left) {
        return -1;
    }

    return bytes_written;
}

/**---------------REQUEST HANDLERS--------------*/
void move_excess_header(connection_t * client_data) {
    size_t excess_bytes = client_data->bytes_read - client_data->header_bytes;

    char * excess_buf = client_data->header + client_data->header_bytes;
    //only try to move filesize if the request was PUT and there's enough bytes
    bool get_filesize = client_data->method == PUT && excess_bytes >= sizeof(size_t);
    
    //move 8 bytes of excess header to filesize buffer
    if (get_filesize) {
        LOG("found filesize in buffer");
        memcpy(client_data->fs_buf, excess_buf, sizeof(size_t));
        client_data->read_fs = true;
        excess_buf += sizeof(size_t);
        excess_bytes -= sizeof(size_t);
    }

    //move all remaining bytes to main data buffer
    if (excess_bytes > 0) {
        LOG("found %zu excess bytes from header read", excess_bytes);
        memcpy(client_data->buf, excess_buf, excess_bytes);
        client_data->buf_offset += excess_bytes;
    }

    //LOG("bytes_read after moving excess header: %zu", client_data->bytes_read)
}

void path_append_temp_dir(connection_t * client_data) {

    int path_len = strlen(temp_dir) + strlen(client_data->filename) + 1;
    char new_fn[path_len + 1];
    strncpy(new_fn, temp_dir, strlen(temp_dir));
    new_fn[strlen(temp_dir)] = '/';
    strncpy(new_fn + strlen(temp_dir) + 1, client_data->filename, strlen(client_data->filename));
    new_fn[path_len] = 0;

    //LOG("new filename is: %s", new_fn);
    strncpy(client_data->filename, new_fn, path_len);
}

void parse_header(connection_t * client_data) {
    //the header should end with a newline, so find # of chars before first newline
    size_t cbnl = strcspn(client_data->header, "\n");
    //if the buffer has filled but there is no newline character, ERR occurred
    if (cbnl == client_data->header_bytes) {
        if (client_data->header_bytes >= 1024) {
            LOG("client sent too many bytes without newline -- no header?")
            client_data->stat = ERR_REQ;
        }

        LOG("didn't find newline character");
        return;
    } 

    size_t verb_len = strcspn(client_data->header, " ");
    char method[verb_len + 1];
    strncpy(method, client_data->header, verb_len);
    method[verb_len] = 0;
    //LOG("verb string is: %s", method);
    verb v = string_to_verb(method);
    client_data->method = v;

    //update header bytes to reflect which bytes are actually part of the header
    //no data loss since bytes read also reflects total bytes in header buf
    client_data->header_bytes = cbnl + 1;

    if (client_data->method == V_UNKNOWN) {
        LOG("could not parse verb %s, bad request.", method);
        client_data->stat = ERR_REQ;
        return;
    }

    if (client_data->method != LIST) {
        size_t fn_len = cbnl - verb_len - 1;

        //LOG("length of filename : %zu", fn_len);
        char filename[fn_len + 1];
        strncpy(filename, client_data->header + verb_len + 1, fn_len);
        filename[fn_len] = 0;

        LOG("parsed filename is: %s", filename);

        //move excess header bytes to appropriate buffers
        move_excess_header(client_data);

        //LOG("length of header: %zu", client_data->header_bytes );
        strncpy(client_data->filename, filename, fn_len);
        client_data->stat = FILE_CHECK;
        
        path_append_temp_dir(client_data);
        
        return;
    }
    
    //skip directly to forming LIST response if applicable
    client_data->stat = RES_HEADER;
    return;
    
}

int file_exists(const char * filename) {
    for (size_t idx = 0; idx < vector_size(file_list); idx++) {
        char * fn = vector_get(file_list, idx);
        if (!strcmp(fn, filename)) {
            return idx;
        }
    }

    return -1;
} 

int create_file(char * filename) {
    //LOG("filename is: %s\n", filename);
    int retval = open(filename, O_CREAT | O_TRUNC, S_IRWXG | S_IRWXO | S_IRWXU);
    
    if (retval == -1) {
        perror("couldn't open file for writing");
        return -1;
    }
    
    vector_push_back(file_list, filename);
    //LOG("no. files: %zu", vector_size(file_list));
    return 0;
}

void check_file(connection_t * client_data) {

    if (client_data->method == GET || client_data->method == DELETE) {
        int valid_file = file_exists(client_data->filename);
        if (valid_file == -1) {
            client_data->stat = ERR_NOFILE;
            LOG("%s does not exist.\n", client_data->filename);
            return;
        }

        switch(client_data->method) {
            case GET:
                client_data->stat = RES_HEADER;
                break;
            case DELETE:
                client_data->stat = DEL_FILE;
                break;
            default:
                break;
        }
    }

    else if (client_data->method == PUT) {
        int made_file = create_file(client_data->filename);
        if (made_file == -1) {
            client_data->stat = ERR; //maybe try again later idk
            return;
        }

        client_data->stat = PARSE_FS;
    }
}

void parse_fs(connection_t * client_data) {
    if (!client_data->read_fs) {
        bool read_fs = read_filesize(client_data);
        if (!read_fs) {
            return;
        }
    }

    
    for (size_t idx = 0; idx < sizeof(size_t); idx++) {
        char c = client_data->fs_buf[idx];
        char * target = (char *)(&client_data->file_size) + idx;
        *target = c;
    }

    client_data->read_fs = true;
    
    client_data->stat = DATA_READ;
}

void write_buffer_to_file(connection_t * client_data) {
    int file_fd = open(client_data->filename, O_RDWR);
    
    if (file_fd == -1) {
        perror("open() :");
        client_data->stat = ERR;
    }

    if (lseek(file_fd, client_data->file_offset, SEEK_SET) == -1) {
        perror("lseek() : ");
        client_data->stat = ERR;
    }

    int remaining_bytes = client_data->buf_offset;
    int bytes_written = 0;
    while (remaining_bytes > 0) {
        int retval = write(file_fd, client_data->buf, client_data->buf_offset);
        
        if (retval == -1) {
            perror("write(): ");
            client_data->stat = ERR;
        }

        int leftover = client_data->buf_offset - retval;
        if (leftover) {
            memmove(client_data->buf, client_data->buf + retval, leftover);
        }

        client_data->buf_offset -= retval;
        remaining_bytes = client_data->buf_offset;
        bytes_written += retval;
    }

    client_data->file_offset += bytes_written;
    close(file_fd);

    
}

void handle_put_request(connection_t * client_data) {
    //loop: write existing buffer data to file, read more bytes into buffer
    //stop when client has closed connection

    //write data to file before attempting to read from socket
    //important as this updates offsets and empties buffer!
    if (client_data->buf_offset > 0) {
        write_buffer_to_file(client_data);
    }
    
    int retval = 0;

    do {
        int rem = buf_size - client_data->buf_offset;
        retval = read_from_socket(client_data, client_data->buf + client_data->buf_offset, rem);
        write_buffer_to_file(client_data);
    } while (retval != -1 && !client_data->closed_read);
    
    if (client_data->closed_read) {
        size_t recd_bytes = client_data->file_offset;
        LOG("received %zu bytes from client", client_data->file_offset);
        if (recd_bytes < client_data->file_size) {
            print_too_little_data();
            client_data->stat = ERR_FS;
        } else if (recd_bytes > client_data->file_size) {
            print_received_too_much_data();
            client_data->stat = ERR_FS;
        } else {
            LOG("received enough data from client");
            //listen for when the client is available to be written to
            epoll_event(client_data->fd, client_data, EPOLL_CTL_MOD, EPOLLOUT);
            client_data->stat = RES_HEADER;
        }
    }
}

/**--------------------RESPONSE HANDLERS------------*/
size_t write_buffer_from_file(connection_t * client_data) {
    size_t bytes_read = 0;
    
    int file_fd = open(client_data->filename, O_RDONLY);

    if (lseek(file_fd, client_data->file_offset, SEEK_SET) == -1) {
        perror("lseek(): ");
        client_data->stat = ERR;
    }
    
    int remaining_space = buf_size - client_data->buf_offset;
    int remaining_bytes = client_data->file_size - client_data->file_offset;

    int n_bytes = remaining_bytes > remaining_space ? remaining_space : remaining_bytes;
    LOG("space in buffer: %d", remaining_space);
    LOG("bytes left to write: %d", remaining_bytes);
    while (bytes_read < (size_t)n_bytes) {
        int retval = read(file_fd, client_data->buf + client_data->buf_offset, n_bytes);
        if (retval == -1) {
            perror("read() : ");
            break;
        }

        bytes_read += retval;
        client_data->buf_offset += retval;
    }

    close(file_fd);
    client_data->file_offset += bytes_read;
    return bytes_read;
}

void send_ok(connection_t * client_data) {
    char * ok = "OK\n";

    int retval = write_to_socket(client_data, ok, strlen(ok));
    
    if (retval == (int)strlen(ok)) {
        switch(client_data->method) {
            case GET:
                client_data->stat = SEND_FS;
                break;
            case PUT:
                client_data->stat = CLOSED;
                break;
            case DELETE:
                client_data->stat = CLOSED;
                break;
            case LIST:
                client_data->stat = SEND_FS;
                break;
            default:
                break;
        }
    }
}

void send_err(connection_t * client_data) {
    char * err = "ERROR\n";

    int retval = write_to_socket(client_data, err, strlen(err));

    char err_msg[255];

    if (retval == (int)strlen(err)) {
        switch(client_data->stat){
            case ERR_REQ:
                strcpy(err_msg, err_bad_request);
                break;
            case ERR_FS:
                strcpy(err_msg, err_bad_file_size);
                break;
            case ERR_NOFILE:
                strcpy(err_msg, err_no_such_file);
                break;
            default:
                LOG("error not otherwise specified");
                break;
        }
    }

    //LOG("err_msg: %s", err_msg);
    retval = write_to_socket(client_data, err_msg, strlen(err_msg));
    //LOG("wrote full errmsg: %d", (int)strlen(err_msg) == retval);
    if (retval == (int)strlen(err_msg)) {
        client_data->stat = CLOSED;
    }
}

void send_fs(connection_t * client_data){
    ssize_t s = 0;

    if (client_data->method == GET) {
         int file_fd = open(client_data->filename, O_RDONLY);
         if (file_fd ==-1) {
            perror("couldn't open(): ");
            client_data->stat = ERR;
        }

        struct stat f_stat;
        fstat(file_fd, &f_stat);
        s =  f_stat.st_size;
        close(file_fd);
        client_data->file_size = s;
    } else if (client_data->method == LIST) {
        s = vector_size(file_list);
    }
   

    int retval = write_to_socket(client_data, (char *)(&s), sizeof(ssize_t));
    if (retval != -1) {
        client_data->stat = DATA_WRITE;
    }

    
}

void handle_get_request(connection_t * client_data) {
    size_t bytes_written = 0;
    LOG("need to write: %zu bytes to client", client_data->file_size);
    //loop - load buffer from file -> write to client -> clear buffer
    while (bytes_written < client_data->file_size && !client_data->closed_write) {
        LOG("writing buffer");
        //load bytes from the file into client buffer
        write_buffer_from_file(client_data);
        
        LOG("writing to socket");
        //write client buffer to client socket
        int retval = write_to_socket(client_data, client_data->buf, client_data->buf_offset);
    
        //retry on write failure
        if (retval == -1) {
            break;
        }

        //update bytes written
        bytes_written += retval;

        //compare buf offset to number of written bytes
        int leftover_bytes = client_data->buf_offset - retval;
        LOG("found %d leftover bytes", leftover_bytes);
        if (leftover_bytes > 0) {
            //move leftover bytes to front of buffer
            memmove(client_data->buf, client_data->buf + retval, leftover_bytes);
        }
        
        LOG("updating buf_offset")
        //update buf offset
        client_data->buf_offset -= retval;
    }
    
    //close connection 
    client_data->stat = CLOSED;
    
}

void handle_list_request(connection_t * client_data) {
    int retval = 0;
    char files_list[2 * buf_size];
    int list_len = 0;
    //iterate over filenames
    for(size_t i = 0; i < vector_size(file_list); i++) {
        //for all elements except the first, add a newline char
        if (i > 0) {
            files_list[list_len] = '\n';
            list_len++;
        }

        //copy filename to list
        char * filename = vector_get(file_list, i);
        strncpy(files_list + list_len, filename, strlen(filename));
        list_len += strlen(filename);
    }
    
    files_list[list_len] = 0;
    
    //write files list to client socket
    size_t bytes_written = 0;
    while (list_len - bytes_written > 0) {
        retval = write_to_socket(client_data, files_list, list_len);
        
        if (retval == -1) {
            continue;
        }

        if (client_data->closed_write) {
            break;
        }

        bytes_written += retval;
    }

    client_data->stat = CLOSED;
}

void handle_del_request(connection_t * client_data) {
    int retval = del_file(client_data->filename);
    if (retval == -1) {
        client_data->stat = ERR;
    }

    client_data->stat = RES_HEADER;
    
}
/**-----------STATE MANAGEMENT-----------*/
void state_switch(connection_t * client_data) {
    switch(client_data->stat) {
            case PENDING:
                read_header(client_data);
                if (client_data->header_bytes > 0) {
                    parse_header(client_data);
                }
                break;
            case FILE_CHECK:
                check_file(client_data);
                break;
            case PARSE_FS:
                parse_fs(client_data);
                break;
            case DEL_FILE:
                handle_del_request(client_data);
                break;
            case DATA_READ:
                handle_put_request(client_data);
                break;
            case RES_HEADER:
                send_ok(client_data);
               break;
            case SEND_FS:
                send_fs(client_data);
                break;
            case DATA_WRITE:
                if (client_data->method == GET) {
                    handle_get_request(client_data);
                } else if (client_data->method == LIST) {
                    handle_list_request(client_data);
                }
                break;
            case ERR_REQ:
                send_err(client_data);
                break;
            case ERR_FS:
                send_err(client_data);
                break;
            case ERR_NOFILE:
                send_err(client_data);
                break;
            case ERR:
                send_err(client_data);
                break;
            case CLOSED:
                close_connection(client_data);
                break;
            default:
                break;
        }
}

void ready_dispatch(struct epoll_event * events, int ready_fds) {
    int idx = 0;
    for (; idx < ready_fds; idx++) {
        struct epoll_event event = events[idx];
        connection_t * client_data = (connection_t *)(event.data.ptr);
        state_switch(client_data);
    }
}

void run_server() {
    while(!endSession) {
        accept_connections();
        if (clientCount) {
            struct epoll_event events[clientCount];
            int ready_fds = epoll_wait(epoll_fd, events, clientCount, 0);
            if (ready_fds == -1) {
                perror("epoll_wait():");
                exit_fail();
            } else if (ready_fds > 0) {
                ready_dispatch(events, ready_fds);
            }
        }
        
    }
}

int main(int argc, char **argv) {
    //verify arguments to program
    if (argc != 2) {
        print_server_usage();
        exit_fail();
    }

    endSession = false;

    //from charming chatroom signal handling
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_server;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    //setup file list vector
    file_list = string_vector_create();

    //setup connection list vector
    connections = shallow_vector_create();
    
    //create temp dir
    char template[7] = "XXXXXX";
    temp_dir = mkdtemp(template);
    LOG("Storing files at %s", temp_dir);
    
    char * port = argv[1];
    setup_server_socket(port);

    //create epoll fd
    epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        perror("epoll_create(): ");
        exit_fail();
    }

    run_server();
}

