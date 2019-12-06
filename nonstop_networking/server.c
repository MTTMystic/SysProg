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
    char * filename;
    /**[only applies for PUT requests] amt of binary data to be transferred*/
    size_t file_size;

    /**buffer for storing header info, including size*/
    char header[buf_size + 1];
    /**how many bytes are in the header*/
    size_t header_bytes;

    char fs_buf[9];

    /**buffer for storing read/write data for files*/
    char buf[buf_size + 1];
    /**stores buffer offset*/
    size_t buf_offset;
    /**bytes written to client*/
    size_t bytes_written;
    /**bytes read from client*/
    size_t bytes_read;
} connection_t;


static char * temp_dir; //folder name of temp dir
static vector * file_list; //global list of files
static dictionary * connections; //global list of client info

static bool endSession; //flag to exit server automata
static volatile int serverSocket; //socket for incoming connections

static volatile int epoll_fd;
static struct epoll_event * ep_events;
static int maxEvents;
static int clientCount;


void cleanup() {
    if (file_list) {
        vector_destroy(file_list);
    }

    if (connections) {
        vector * conn_t_vec = dictionary_values(connections);
        for (size_t idx = 0; idx < vector_size(conn_t_vec); idx++) {
            connection_t * conn_t = vector_get(conn_t_vec, idx);
            shutdown(conn_t->fd, SHUT_RDWR);
            close(conn_t->fd);
            free(conn_t);
        }
        dictionary_destroy(connections);
    }

    if (temp_dir) {
        rmdir(temp_dir);
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

void path_append_temp_dir(connection_t * client_data) {
    char * filename = client_data->filename;
    //alloc new buffer of [fn length + temp dir length +  byte for /] bytes
    int new_path_len = strlen(filename) + strlen(temp_dir) + 1;

    char * new_filename = malloc(new_path_len + 1);
    strncpy(new_filename, temp_dir, strlen(temp_dir));
    new_filename[strlen(temp_dir)] = '/';
    strncpy(new_filename + strlen(temp_dir) + 1, filename, strlen(filename));
    new_filename[new_path_len] = 0;
    client_data->filename = new_filename;

    LOG("new filename is: %s", new_filename);
}

void check_filename(connection_t * client_data) {
    bool checkFileExists = client_data->method == GET || client_data->method == DELETE;
    
    if (checkFileExists) {
        if (access(client_data->filename, R_OK) == -1) {
            client_data->stat = ERR_NOFILE;
            LOG("client tried to get/delete file that doesn't exist");
            return;
        }
        //move to next state
        //for get requests, send response header
        //for delete requests, delete the file
        client_data->stat = (client_data->method == GET) ? RES_HEADER : DEL_FILE;
    } else if (client_data->method == PUT) {
        int temp_fd = open(client_data->filename, O_CREAT | O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);
        if (temp_fd >= 0) {
            LOG("opened file for writing");
            client_data->stat = PARSE_FS;
        } else {
            perror("could not open/create file for PUT request: ");
            client_data->stat = ERR;
        }
    }
}


//copy filesize into buffer after reading into header (if excess exists)
//then operate only on buffer for initial reads!
 
/*attempts to read up to [n_bytes] bytes from socket into given buffer
 ASSUMES that buf has enough room for n_bytes + 1 (null terminator)*/
int read_from_socket(int fd, char * buf, size_t n_bytes) {
    size_t bytes_read = 0;
    while (bytes_read < n_bytes) {
        int retval = read(fd, buf + bytes_read, n_bytes - bytes_read);
        
        if (retval == 0) {
            LOG("connection closed or no data available on %d", fd);
            break;
        }

        if (retval == -1 && errno == EINTR) {
            LOG("read was interrupted.");
            continue;
        }

        if (retval == -1) {
            LOG("read() on %d :", fd);
            return -1;
        }
        
        LOG("read %d bytes from %d", retval, fd);
    
        bytes_read += retval;
    }

    return bytes_read;
}

void read_header(connection_t * client_data) {
    char * header = client_data->header + client_data->header_bytes;
    int remaining_bytes = sizeof(client_data->header) - client_data->header_bytes - 1;
    LOG("bytes remaining in header: %d", remaining_bytes);
    int bytes_read = read_from_socket(client_data->fd, header, remaining_bytes);
    client_data->header_bytes += bytes_read;
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
    LOG("verb string is: %s", method);
    verb v = string_to_verb(method);
    client_data->method = v;

    if (client_data->method == V_UNKNOWN) {
        LOG("could not parse verb %s, bad request.", method);
        client_data->stat = ERR_REQ;
        return;
    }

    if (client_data->method != LIST) {
        size_t fn_len = cbnl - verb_len - 1;
        LOG("length of filename : %zu", fn_len);
        char filename[fn_len + 1];
        strncpy(filename, client_data->header + verb_len + 1, fn_len);
        filename[fn_len] = 0;

        LOG("parsed filename is: %s", filename);

        client_data->filename = filename;
        client_data->stat = FILE_CHECK;
        return;
    }
    
    //skip directly to forming LIST response if applicable
    client_data->stat = RES_HEADER;
    return;
    
}

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

/**helper function to double the size of the epoll events arr*/
void resize_events_arr() {
    struct epoll_event * ep_events_new = realloc(ep_events, 2 * sizeof(ep_events));
    if (ep_events_new) {
        ep_events = ep_events_new;
        maxEvents *= 2;
    } else {
        perror("realloc(): ");
    }
}

connection_t * connection_setup(int fd) {
    connection_t * new_conn = (connection_t *) malloc(sizeof(connection_t));
    if (!new_conn) {
        perror("Failed to allocate memory for new connection.");
        exit_fail();
    }

    new_conn->fd = fd;
    new_conn->stat = PENDING;
    new_conn->buf[1024] = 0;
    new_conn->header[1024] = 0;
    new_conn->file_size = pow(2, 8) - 1;
    new_conn->header_bytes = 0;
    new_conn->buf_offset = 0;
    dictionary_set(connections, (void *)&fd, (void *)new_conn);

    return new_conn;
}

void epoll_add_event(int fd, void * data_ptr, uint32_t flags) {
    struct epoll_event ev;
    ev.events = flags;
    ev.data.fd = fd;
    ev.data.ptr = data_ptr;

    if (clientCount > maxEvents) {
        resize_events_arr();
    }

    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (ret != -1) {
        ep_events[clientCount - 1] = ev;
    } else {
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
            epoll_add_event(new_cli_fd, (void*)new_conn, EPOLLIN);
        }
      
   } while (new_cli_fd != -1);
}

void state_switch(connection_t * client_data) {
    switch(client_data->stat) {
            case PENDING:
                read_header(client_data);
                if (client_data->header_bytes > 0) {
                    parse_header(client_data);
                }
                break;
            case FILE_CHECK:
                //check_filename(client_data);
                break;
            case PARSE_FS:
                //read_filesize(client_data);
                break;
            case DEL_FILE:
            case DATA_READ:
                //write_excess_data(client_data);
                break;
            case RES_HEADER:
            case SEND_FS:
            case DATA_WRITE:
            case ERR_REQ:
            case ERR_FS:
            case ERR_NOFILE:
            case ERR:
            default:
                break;
        }
}

void ready_dispatch(struct epoll_event * events) {
    int idx = 0;
    for (; idx < clientCount; idx++) {
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
                ready_dispatch(events);
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
    
    maxEvents = 10;
    ep_events = calloc(maxEvents, sizeof(struct epoll_event));

    //setup file list vector
    file_list = string_vector_create();

    //setup connection list vector
    connections = int_to_shallow_dictionary_create();
    
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

