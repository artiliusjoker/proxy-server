#include "../include/proxy.h"

// Signal handling
void SIGUSR1_handler(int signum)
{

}
void SIGUSR2_handler(int signum)
{
    pid_t wait_retVal;
    int status;
    fprintf(stdout, "\nShutting down all processes\n");

    int killall = 0 - getpid();
    kill(killall, SIGINT);

    while ((wait_retVal = wait(&status)) > 0);
    fprintf(stdout, "Program terminated by SIGUSR2\n");
    
    exit(EXIT_SUCCESS);
}
static void TERM_handler(int signum)
{
    fprintf(stdout, "Shut down process #%d\n", getpid());
    exit(EXIT_SUCCESS);
}

static void start_proxy_server(char *port);
static void handle_client(int client_fd);

int main(int argc, char *argv[])
{
    start_proxy_server(argv[1]);
    wait(NULL);
    exit(EXIT_SUCCESS);
}

static void start_proxy_server(char *port){
    // SIGNAL handler settings
    signal(SIGUSR2, SIGUSR2_handler);
    signal(SIGINT, SIG_IGN);
    //signal(SIGTERM, SIG_IGN);

    struct addrinfo result, *list;
    int fd;

    // Allocate mem for result
    memset(&result, 0, sizeof(result));
    result.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    result.ai_socktype = SOCK_STREAM; /* Datagram socket */
    result.ai_flags = AI_PASSIVE;

    int retVal = getaddrinfo(NULL, port, &result, &list);
    if (retVal != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retVal));
        exit(EXIT_FAILURE);
    }
    struct addrinfo *iterator = list;
    if(iterator == NULL)
    {
        fprintf(stderr, "Server: cannot create socket fd !\n");
        return;
    }
    // Go through the list to create socket fd
    for(iterator; iterator != NULL; iterator = iterator->ai_next)
    {
        if((fd = socket(iterator->ai_family, iterator->ai_socktype,
                        iterator->ai_protocol)) < 0)
        {
            continue;
        }

        if(bind(fd, iterator->ai_addr, iterator->ai_addrlen) < 0)
        {
            close(fd);
            continue;
        }
        break;
    }
    freeaddrinfo(list);
    
    // Prepare for connections
    if(listen(fd, MAX_REQUESTS_QUEUE_SIZE) < 0)
    {
        perror("Server ");
        return;
    }
    int accept_fd;
    // Fork process's child to handle clients
    printf("Proxy server is listening on port %s\n", port);
    while(1)
    {
        accept_fd = accept(fd, NULL, NULL);
        if(accept_fd == -1)
        {
            perror("Server ");
            continue;
        }

        printf("Received connection\n");

        pid_t child_pid = fork();
        
        // Parent process
        if(child_pid > 0)
        {
            break;
        }
        // Child process
        if(child_pid == 0)
        {
            signal(SIGINT, TERM_handler);
            handle_client(accept_fd);
            // Child process exits
            close(accept_fd);
            exit(EXIT_SUCCESS);
        }
    }
}

static void handle_client(int client_fd){
    
    int server_fd, retVal;
    http_request *client_request = NULL;
    char * request_in_string;

    // Read client's request
    client_request = http_read_request(client_fd, &request_in_string);
    if(client_request == NULL)
    {
        fprintf(stderr, "Handling : cannot read client's request ! \n");
        return;
    }

    // Filter URLs
    
    // Filter methods

    // Connect to web server
    
    server_fd = connect_server(client_request);
    if(server_fd == -1)
    {
        http_request_free(client_request);
        free(request_in_string);
        return;
    }

    retVal = send_request(server_fd, request_in_string);
    if(retVal < 0)
    {
        close(server_fd);
        free(request_in_string);
        http_request_free(client_request);
        return;
    }

    int is_bad_encoding = 0;
    int is_text_content = 0;
    // Buffer to read line from socket
    char buffer[MAX_LINE_BUF];
    read_buffer *rbuf;
    rbuf = calloc(1, sizeof(*rbuf));
    rbuf->current_fd = server_fd;

    while(1)
    {   
        read_line_socket(rbuf, buffer, MAX_LINE_BUF);
        int line_length = strlen(buffer);
        retVal = send_line(client_fd, buffer);

        if(buffer[0] == '\r' && buffer[1] == '\n')
        {
            // We received the end of the HTTP header
            break;
        }
    }
    free(rbuf);
    close(server_fd);
    free(request_in_string);
    http_request_free(client_request);
}