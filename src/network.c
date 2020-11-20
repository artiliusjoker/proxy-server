#include "../include/proxy.h"

//from Beej's guide
int send_all_to_socket(int sock_fd, char *buf, int buf_len, int *send_len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = buf_len; // how many we have left to send
    int n;

    while(total < buf_len) {
        n = send(sock_fd, buf + total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    if(send_len != NULL)
    {
        *send_len = total; // return number actually sent here
    }
    return n == -1 ? - 1 : 0; // return -1 on failure, 0 on success
} 

// Network function
int connect_server(http_request *request)
{
    char *host = (char*)list_get_value(&request->metadata_head, "Host"); 
    char *port = strstr(host, ":");
    int flag = 0;

    if(port == NULL)
    {
        // set port to default
        port = calloc(3, sizeof(char));
        strcpy(port, "80");
        flag = 1;
    }
    else
    {
        // remove the port number from the host
        host = strtok(host, ":");
        // jump over the ':' char
        port++;
    }
    
    // Create and connect the socket to web server
    struct addrinfo hints, *servinfo, *p, *holder; 
	int sockfd, rv; 

	memset(&hints, 0, sizeof(hints)); 
	hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_STREAM; 
    // DNS lookup to get IP address from hostname
	if((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0)
	{
        free(port);
		return -1; 
	}
    if(flag)
    {
        free(port);
    }
	// loop through all the results and connect to the first we can
    p = servinfo;
    holder = p;
    if (p == NULL) 
    {
        
		return -1;
	}
	for(p; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) 
        {
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
			close(sockfd);
			continue;
		}
		break;
	}
    freeaddrinfo(holder);
    if (p == NULL) 
    {
		return -1;
	}
    return sockfd;
}
int send_request(int server_fd, char* request_in_string)
{
    if(send_all_to_socket(server_fd, request_in_string, strlen(request_in_string), NULL) < 0)
    {
        perror("Sending ");
        return -1;
    }
    return 0;
}

int receive_and_reply_content(int server_fd, int client_fd)
{
    ssize_t num_bytes_read = 0;
    char* recv_buf;
    int err_flag = 0;
    

    
    while(1)
	{
        recv_buf = (char *) malloc(MAX_READ_BUF);
		num_bytes_read = recv(server_fd, recv_buf, MAX_READ_BUF, 0);
        
         // No more bytes to read
		if(num_bytes_read <= -1) 
		{
            err_flag = 1;
			break;
		}
		else if(num_bytes_read == 0)
		{
			break;
		}
        if(num_bytes_read < MAX_READ_BUF)
        {
            // Bytes received might less than allocated buffer
            // So overlap it with smaller allocated memory
            char* buf_hold = (char *) malloc(num_bytes_read);
            memcpy(buf_hold, recv_buf, num_bytes_read);
            free(recv_buf);
            //memcpy(recv_buf, buf_hold, num_bytes_read);
            recv_buf = buf_hold;
            buf_hold = NULL;
        }
        // Send the recv_buffer to client
		send_all_to_socket(client_fd, recv_buf, num_bytes_read, NULL);
        free(recv_buf);

	}
    free(recv_buf);
    if(err_flag)
    {
        return -1;
    }
    return 0;
}

int send_error_response(int error_code, int client_fd)
{
    http_custom_response * error_response = http_response_build(error_code);
    send_all_to_socket(client_fd, error_response->http_header, error_response->header_size, NULL);
    http_response_free(error_response);
    return 0;
}