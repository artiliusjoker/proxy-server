#include "../include/http.h"

typedef struct r_buf{
    char buf[MAX_READ_BUF];
    unsigned int r_pos, w_pos; // for keeping track of positions when read line
}read_buffer;

int read_line_socket(int fd, char *dst, unsigned int size)
{
    unsigned int i = 0;
    ssize_t bytes_recv;
    read_buffer buffer;

    while (i < size) {
        if (buffer.r_pos == buffer.w_pos) 
        {
            size_t wpos = buffer.w_pos % MAX_READ_BUF;
        
            if((bytes_recv = recv(fd, buffer.buf + wpos, (MAX_READ_BUF - wpos), 0)) < 0) 
            {
                return -1;
            } 
            else if (bytes_recv == 0)
            {
                 return 0;
            }
            buffer.w_pos += bytes_recv;
        }
        dst[i++] = buffer.buf[buffer.r_pos++ % MAX_READ_BUF];
        if (dst[i - 1] == '\n')
        {
            break;
        }
    }
    // Line too big
    if(i == size) 
    {
        return -1;
    }
    // String terminate char
    dst[i] = '\0';
    return i;
}

void http_read_header(int sockfd, http_request *new_request)
{
	new_request = (http_request *) malloc(sizeof(http_request));
    new_request->method = 0; 
    TAILQ_INIT(&new_request->metadata_head); 

	char line[MAX_LINE_BUF]; 
	//http_parse_request_method(req, line); 

    do
    {
       read_line_socket(sockfd, line, MAX_LINE_BUF);
       if(line[0] == '\r' && line[1] == '\n')
		{
			// The end of the HTTP header 
			break; 
		}
        fprintf(stdout, "%s\n", line);

    } while (1);
    //http_request_destroy(new_request);
}

void http_request_destroy(http_request *req)
{
    struct http_metadata_item *item; 
    TAILQ_FOREACH(item, &req->metadata_head, entries) {
        free((char*)item->key);
        free((char*)item->value); 
        free(item);
    }
}