#include "../include/proxy.h"

int main(int argc, char *argv[])
{
    start_proxy_server(DEFAULT_PORT);
    return 0;
}

static void start_proxy_server(int port){
    struct addrinfo result, *list;
    int fd;

    // Allocate mem for result
    memset(&result, 0, sizeof(struct addrinfo));
    result.ai_family = AF_UNSPEC; // any address family
    result.ai_socktype = SOCK_STREAM; // TCP
    result.ai_flags = AI_PASSIVE; //binding

    int flag;
    if((flag = getaddrinfo(NULL, port, &result, &list)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(flag));
        return;
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
                        iterator->ai_protocol)) == -1)
        {
            continue;
        }

        if(bind(fd, iterator->ai_addr, iterator->ai_addrlen) == -1)
        {
            close(fd);
            continue;
        }
        break;
    }
    freeaddrinfo(list);

    // Prepare for connections
    if(listen(fd, MAX_REQUESTS_QUEUE_SIZE) == -1)
    {
        perror("Server : ");
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

        printf("Receieved connection\n");

        signal(SIGCHLD, SIG_IGN);
        
        pid_t child_pid = fork();

        // Handling process starts
        if(!child_pid)
        {
            handle_client(accept_fd);
            // Handling process exits
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
    http_read_header(client_fd, client_request);
    if(client_request == NULL)
    {
        fprintf(stderr, "Handling : cannot read client's request ! \n");
        return;
    }
    // Filter URLs
    
    // Filter methods

    // Connect to web server
    server_fd = http_connect(client_request);
    if(server_fd == -1)
    {
        http_request_destroy(client_request);
        return;
    }
    close(server_fd);
}