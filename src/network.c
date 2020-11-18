#include "../include/proxy.h"

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
    struct addrinfo hints, *servinfo, *p; 
	int sockfd, rv; 

	memset(&hints, 0, sizeof(hints)); 
	hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_STREAM; 
    // DNS lookup to get IP address from hostname
	if((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0)
	{
		return -1; 
	}
	// loop through all the results and connect to the first we can
    p = servinfo;
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
    if (p == NULL) 
    {
		return -1;
	}
    return sockfd;
}
int send_request(int server_fd, char* request_in_string)
{
    // send the http request to the web server
    if(send(server_fd, request_in_string, strlen(request_in_string), 0) == -1)
    {
        perror("Sending ");
        return -1;
    }
    return 0;
}
//int receive_and_reply();
//int send_reply();
int send_line(int client_fd, char*line)
{
    if(send(client_fd, line, strlen(line), 0) == -1)
    {
        perror("Sending to client ");
        return -1;
    }
    return 0;
}