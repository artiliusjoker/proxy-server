#ifndef HTTP_H
#define HTTP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "constants.h"

enum http_method{
    OPTIONS,
    GET,
    HEAD,
    POST,
    PUT, 
    DELETE, 
    TRACE,
    CONNECT, 
    UNKNOWN
};

enum http_version{
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
    HTTP_VERSION_INVALID
};

typedef enum http_method http_method;
typedef enum http_version http_version;

// Struct to store other data of http request
typedef struct http_metadata_item
{ 
    const char *key; 
    const char *value;
    // Pointers to previous and next entries
    TAILQ_ENTRY(http_metadata_item) entries;
}http_metadata_item; 

// Struct to store raw request from client
typedef struct http_request
{
    http_method method; 
    http_version version;
    const char *url;
    TAILQ_HEAD(METADATA_HEAD, http_metadata_item) metadata_head; 
} http_request;

// Cleaning struct http_request
void http_request_free(http_request*);

// Read raw request from client and store in struct http_request (machine-friendly) 
// and build the request in char* buffer
http_request *http_read_request(int, char**);

// Build raw request from struct http_request to send to server
char *http_build_request(http_request*);
// Send error reply to clients
char *http_build_reply();

// Utils
// Read a whole line from fd
typedef struct r_buf{
    char buf[MAX_READ_BUF];
    unsigned int r_pos, w_pos; // for keeping track of positions when read line
    unsigned int current_fd;
}read_buffer;
int read_line_socket(read_buffer *, char *, unsigned int);

#endif