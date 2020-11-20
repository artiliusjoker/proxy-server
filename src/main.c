#include "../include/proxy.h"

static void print_proxy_status();

// Global var for printing program status
int processed_requests;
int filtered_requests;
int error_requests;
char *filter_requests;
int listening_fd;

// Signal handling
void SIGUSR1_handler(int signum)
{
    print_proxy_status();
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
    
    close(listening_fd);
    if(filter_requests)
        free(filter_requests);
    exit(EXIT_SUCCESS);
}
static void TERM_handler(int signum)
{
    fprintf(stdout, "Shut down process #%d\n", getpid());
    exit(EXIT_SUCCESS);
}

static void start_proxy_server(char *port);
static void handle_client(int client_fd, char *);
static char filter_check(char *filter, char*url_to_check);

int main(int argc, char *argv[])
{
    processed_requests = 0;
    filtered_requests = 0;
    error_requests = 0;
    listening_fd = 0;

    if(argv[2] != NULL)
    {
        filter_requests = strdup(argv[2]);
    }

    start_proxy_server(argv[1]);
    wait(NULL);

    if(filter_requests)
        free(filter_requests);
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
        printf("Error in listen\n");
        perror("Server ");
        return;
    }
    listening_fd = fd;
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
        // if(child_pid > 0)
        // {
        //     break;
        // }
        // Child process
        if(child_pid == 0)
        {
            signal(SIGINT, TERM_handler);
            handle_client(accept_fd, filter_requests);
            // Child process exits
            close(accept_fd);
            exit(EXIT_SUCCESS);
        }
    }
}

static void handle_client(int client_fd, char *filter_str){
    
    int server_fd, retVal;
    http_request *client_request = NULL;
    char * request_in_string;

    // Read client's request
    client_request = http_read_request(client_fd, &request_in_string);
    if(client_request == NULL)
    {
        send_error_response(BAD_REQUEST, client_fd);

        fprintf(stderr, "Handling : cannot read client's request ! \n");
        return;
    }

    // Filter URLs
    http_parsed_url *parsed_request_url = NULL;
    if(filter_str)
    {
        char *host = (char*)list_get_value(&client_request->metadata_head, "Host");
        //parsed_request_url = parse_url(client_request->search_path);
        char is_filtered = filter_check(filter_str, host);
        if(is_filtered == 'F')
        {
            send_error_response(FORBIDDEN, client_fd);
            http_request_free(client_request);
            free(request_in_string);
            return;
        }
    }

    // Filter methods
    if(client_request->method != HEAD && client_request->method != GET)
    {
        send_error_response(METHOD_NOT_ALLOWED, client_fd);

        http_request_free(client_request);
        free(request_in_string);
        return;
    }

    // Connect to web server
    server_fd = connect_server(client_request);
    if(server_fd == -1)
    {
        send_error_response(NOT_FOUND, client_fd);

        http_request_free(client_request);
        free(request_in_string);
        return;
    }
    // Send request to web server
    retVal = send_request(server_fd, request_in_string);
    if(retVal < 0)
    {
        send_error_response(NOT_FOUND, client_fd);

        close(server_fd);
        http_request_free(client_request);
        free(request_in_string);
        return;
    }
    free(request_in_string);
    
    // // Read header from socket, if METHOD == HEAD => no content available from web server => stop, only read response header
    // char buffer[MAX_LINE_BUF];
    // read_buffer *rbuf;
    // rbuf = calloc(1, sizeof(*rbuf));
    // rbuf->current_fd = server_fd;

    // while(1)
    // {   
    //     int check = read_line_socket(rbuf, buffer, MAX_LINE_BUF);
    //     printf("%d\n", check);
    //     int line_length = strlen(buffer);
    //     retVal = send_line(client_fd, buffer);
    //     // End of HTTP reponse header
    //     if(buffer[0] == '\r' && buffer[1] == '\n')
    //     {
    //         // We received the end of the HTTP header
    //         break;
    //     }
    // }
    // free(rbuf);

    // if METHOD == HEAD => no content available from web server => set timeout shorter to break block recv()
    struct timeval tv;
    tv.tv_sec = client_request->method == HEAD ? MAX_RECV_TIMEOUT_HEAD : MAX_RECV_TIMEOUT_GET;
    tv.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    // Receive and send content to client
    receive_and_reply_content(server_fd, client_fd);

    // Done, clean garbage
    close(server_fd);
    http_request_free(client_request);
}

static void print_proxy_status()
{
    fprintf(stdout, "Received SIGUSR1...reporting status:\n"
                    "-- Processed %d requests successfully\n"
                    "-- Filtering: %s\n"
                    "-- Filtered %d requests"
                    "-- Encountered %d requests in error"
                    ,processed_requests, filter_requests, filtered_requests, error_requests);
}

static char filter_check(char *filter, char*url_to_check){

    char *p_substr = strstr(url_to_check, filter);
    int is_suffix = 0;
    int is_prefix = 0;
    // Exists filter in url
    if(p_substr)
    {
        int url_len = strlen(url_to_check);
        // find prefix
        for (size_t i = 0; i < url_len; i++)
        {
            if(url_to_check[i] == '.')
                break;
            if(url_to_check[i] != filter[i])
                is_prefix = 1;
        }
        // find last suffix of filter
        int k = 0;
        for (size_t i = url_len; i >= 0; --i)
        {
            if(filter[i])
            {
                k = i;
                break;
            }
        }
        // find suffix
        for (size_t i = url_len - 1; i >= 0; --i)
        {
            if(url_to_check[i] == '.')
                break;
            if(url_to_check[i] != filter[k])
            {
                is_suffix = 1;
            }
            --k;
        }
        return (is_prefix == 1) && (is_suffix == 1) ? 'T' : 'F';
    }
    return 'F';
}