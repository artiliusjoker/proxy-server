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

http_request *http_read_request(int sockfd, char** request_result)
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

    // copy first line into string
    *request_result = (char *) malloc(strlen(buffer) + 1);
    strcpy(*request_result, buffer);

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
                new_request->search_path = strdup(token);
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
        read_line_socket(rbuf, buffer, MAX_LINE_BUF);
        // copy first line into string
        int buf_len = strlen(buffer);
        int curr_len = strlen(*request_result);
        *request_result = (char *) realloc(*request_result, (buf_len + curr_len) + 1);
        strcat(*request_result, buffer);
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
    //http_request_print(new_request);
    return new_request;
}

void http_request_free(http_request *req)
{
    free((char*)req->search_path);
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
            req->search_path); 
    printf("[Metadata] \n"); 
    struct http_metadata_item *item; 
    TAILQ_FOREACH(item, &req->metadata_head, entries) {
        printf("%s: %s\n", item->key, item->value); 
    }
}
// Tuple contains : code and name of http status
typedef struct http_status_tuple{
    char status_code[4];
    char status_name[20]; 
}http_status_tuple;

http_status_tuple * create_response_tuple(int status_code)
{
    http_status_tuple * tuple = (http_status_tuple *) malloc(sizeof(*tuple));
    switch (status_code)
    {
    case OK:
    {
        strcpy(tuple->status_code, "200");
        strcpy(tuple->status_name, "Ok");
        break;
    }       
    case BAD_REQUEST:
    {
        strcpy(tuple->status_code, "400");
        strcpy(tuple->status_name, "Bad Request");
        break;
    }
    case MOVED_PERMANENTLY:
    {
        strcpy(tuple->status_code, "301");
        strcpy(tuple->status_name, "Moved Permanently");
        break;
    }
    case FORBIDDEN:
    {
        strcpy(tuple->status_code, "403");
        strcpy(tuple->status_name, "Forbidden");
        break;
    }
    case NOT_FOUND:
    {
        strcpy(tuple->status_code, "404");
        strcpy(tuple->status_name, "Not Found");
        break;
    }
    case METHOD_NOT_ALLOWED:
    {
        strcpy(tuple->status_code, "405");
        strcpy(tuple->status_name, "Method Not Allowed");
        break;
    }   
    default:
        free(tuple);
        tuple = NULL;
        break;
    }
    return tuple;
}


http_custom_response *http_response_build(int status_code)
{
    http_custom_response *new_response = (http_custom_response *) calloc(1, sizeof(*new_response));
    new_response->http_header = (char *) malloc(MAX_HTTP_HDR_SIZE);
    new_response->http_html_content = NULL;
    http_status_tuple * tuple;
    tuple = create_response_tuple(status_code);
    
    int retVal = snprintf(new_response->http_header, MAX_HTTP_HDR_SIZE, 
                                                            "HTTP/1.1 %s %s\r\n"
                                                            "Date: Mon, 19 Nov 2020 10:21:21 GMT\r\n" // fake date
                                                            "Cache-Control: no-cache, private\r\n" //No cache
                                                            "Content-Length: 0\r\n" // no content                                                
                                                            "Connection: closed\r\n"
                                                            "\r\n", tuple->status_code, tuple->status_name);
    new_response->header_size = strlen(new_response->http_header);
    new_response->content_size = 0;
    if(retVal < 0)
    {
        http_response_free(new_response);
    }
    free(tuple);
    return new_response;
}
void http_response_free(http_custom_response * response)
{
    if((response)->http_header)
        free((response)->http_header);

    if((response)->http_html_content)
        free((response)->http_html_content);

    if(response){
        free(response);
        response = NULL;
    }
}
