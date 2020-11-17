#include "../include/proxy.h"

// Child list
int child_pids[MAX_REQUESTS_QUEUE_SIZE];
int n_child = 0;

// Signal handling
void SIGUSR1_handler(int signum)
{

}
void SIGUSR2_handler(int signum)
{

}
void SIGINT_handler(int signum)
{
    // for (size_t i = 0; i < MAX_REQUESTS_QUEUE_SIZE; i++)
    // {
    //     if(child_pids[n_child] > 0)
    //     {
    //         kill(child_pids[n_child], SIGTERM);
    //     }
    // }
    pid_t wait_pid;
    int status;
    fprintf(stdout, "\nShutting down all processes\n");
    kill(0, SIGTERM);
    while ((wait_pid = wait(&status)) > 0);
    fprintf(stdout, "Program terminated by CTRL C\n");
    exit(EXIT_SUCCESS);
}

static void TERM_handler(int signum)
{
    fprintf(stdout, "Shut down process #%d\n", getpid());
    exit(2);
}

static void start_proxy_server(char *port);
static void handle_client(int client_fd);

int main(int argc, char *argv[])
{
    memset(child_pids, 0, sizeof(child_pids));
    start_proxy_server(argv[1]);
    return 0;
}

static void start_proxy_server(char *port){
    signal(SIGINT, SIGINT_handler);
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
    
    while(1)
    {
        accept_fd = accept(fd, NULL, NULL);
        if(accept_fd == -1)
        {
            perror("Server : ");
            continue;
        }

        printf("Received connection\n");

        pid_t child_pid = fork();
        
        // Parent process
        if(child_pid > 0)
        {
            child_pids[n_child] = child_pid;
            n_child = n_child + 1;
            continue;
        }

        // Child process job
        if(child_pid == 0)
        {
            signal(SIGTERM, TERM_handler);
            handle_client(accept_fd);
            // Child process exits
            close(accept_fd);
            exit(0);
        }
    }
}

static void handle_client(int client_fd){
    
    char *line;
    int server_fd;
    http_request *client_request = NULL;

    // Read client's request
    client_request = http_read_request(client_fd);
    if(client_request == NULL)
    {
        fprintf(stderr, "Handling : cannot read client's request ! \n");
        return;
    }
    free(client_request);
    // Filter URLs
    
    // Filter methods

    // Connect to web server
    
    // server_fd = http_connect(client_request);
    // if(server_fd == -1)
    // {
    //     http_request_destroy(client_request);
    //     return;
    // }
    // close(server_fd);
}