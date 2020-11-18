#include "../include/http.h"

const int http_method_len = UNKNOWN - OPTIONS;
const char *http_methods_array[] = {
    "OPTIONS", 
    "GET", 
    "HEAD", 
    "POST", 
    "PUT", 
    "DELETE", 
    "TRACE", 
    "CONNECT",
    "UNKNOWN"
};

int read_line_socket(read_buffer *r_buf, char *dst, unsigned int size)
{
    unsigned int i = 0;
    ssize_t bytes_recv;
    
    while (i < size) {
        if (r_buf->r_pos == r_buf->w_pos) 
        {
            size_t wpos = r_buf->w_pos % MAX_READ_BUF;
        
            if((bytes_recv = recv(r_buf->current_fd, r_buf->buf + wpos, (MAX_READ_BUF - wpos), 0)) < 0) 
            {
                return -1;
            } 
            else if (bytes_recv == 0)
            {
                 return 0;
            }
            r_buf->w_pos += bytes_recv;
        }
        dst[i++] = r_buf->buf[r_buf->r_pos++ % MAX_READ_BUF];
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

static void http_request_print(http_request *req);

http_request *http_read_request(int sockfd)
{
	http_request *new_request = (http_request *) malloc(sizeof(http_request));
    new_request->method = 0; 
    TAILQ_INIT(&new_request->metadata_head);

    char buffer[MAX_LINE_BUF];
    read_buffer *rbuf;
    rbuf = calloc(1, sizeof(*rbuf));
    rbuf->current_fd = sockfd;
    
    // First line -> get METHOD, URL, VERSION
    read_line_socket(rbuf, buffer, MAX_LINE_BUF);
    char* token = NULL;
    char* copy, *pointer_copy;

    copy = strdup(buffer);
    pointer_copy = copy;
    int index = 0;
    while ((token = strsep(&copy, " \r\n")) != NULL) {
        if(index == 3) break;
        switch (index) {
            case 0: {
                int found = 0;
                for (int i = 0; i <= http_method_len; i++) {
                    if (strcmp(token, http_methods_array[i]) == 0) {
                        found = 1;
                        new_request->method = i;
                        break;
                    }
                }
                if (found == 0) {
                    new_request->method = http_method_len;
                    free(copy);
                    return new_request;
                }
                ++index;
                break;
            }
            case 1:
                new_request->url = strdup(token);
                ++index;
                break;
            case 2:
            {
                if(strcmp(token, "HTTP/1.0") == 0) 
                {
                    new_request->version = HTTP_VERSION_1_0;
                }     
                else if(strcmp(token, "HTTP/1.1") == 0) 
                {
                    new_request->version = HTTP_VERSION_1_1;
                } 
                else 
                {
                    new_request->version = HTTP_VERSION_INVALID;
                }
                ++index;
                break;
            }
            case 3:
                break;
        }
    }
    // Done first line

    // Clean up
    free(pointer_copy);


    // Rest : other metadata
    do
    {
        int read_size = read_line_socket(rbuf, buffer, MAX_LINE_BUF);
        if(buffer[0] == '\r' && buffer[1] == '\n')
		{
			// The end of the HTTP header
			break;
		}
        copy = strdup(buffer);
        char *key = strdup(strtok(copy, ":"));
        char *value = strdup(strtok(NULL, "\r"));
        free(copy);
        // remove whitespaces
        char *p, *hold;
        p = strdup(value);
        hold = p;
        while(*p == ' ') p++;
        free(value);
        value = strdup(p);
        free(hold);
        // create the http_metadata_item object
        struct http_metadata_item *item = malloc(sizeof(*item));
        item->key = key;
        item->value = value;
        // add the new item to the list of metadatas     
        TAILQ_INSERT_TAIL(&new_request->metadata_head, item, entries);

    } while (1);
    free(rbuf);
    http_request_print(new_request);
    return new_request;
}

void http_request_free(http_request *req)
{
    free((char*)req->url);
    struct http_metadata_item *item; 
    TAILQ_FOREACH(item, &req->metadata_head, entries) {
        free((char*)item->key);
        free((char*)item->value); 
        free(item);
    }
    free(req);
    req = NULL;
}

static void http_request_print(http_request *req)
{
    printf("[HTTP_REQUEST] \n"); 

    switch (req->version) {
      case HTTP_VERSION_1_0:
        printf("version:\tHTTP/1.0\n");
        break;
      case HTTP_VERSION_1_1:
        printf("version:\tHTTP/1.1\n");
        break;
      case HTTP_VERSION_INVALID:
        printf("version:\tInvalid\n");
        break;
    }

    printf("method:\t\t%s\n", 
            http_methods_array[req->method]);
    printf("path:\t\t%s\n", 
            req->url); 
    printf("[Metadata] \n"); 
    struct http_metadata_item *item; 
    TAILQ_FOREACH(item, &req->metadata_head, entries) {
        printf("%s: %s\n", item->key, item->value); 
    }
}