#ifndef HTTP_H
#define HTTP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>

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

typedef struct http_request
{
    http_method method; 
    http_version version;

    TAILQ_HEAD(METADATA_HEAD, http_metadata_item) metadata_head; 
} http_request;

// Struct to store other data of http request
typedef struct http_metadata_item
{ 
    const char *key; 
    const char *value;
    // Pointers to previous and next entries
    TAILQ_ENTRY(metadata_item) entries;
} metadata_item; 

void http_request_init(http_request**);
void http_request_free(http_request*);

void http_parse_request_method(http_request*, char*);
void http_parse_request_metadata(http_request*, char*);


void http_read_header(int, http_request *);
// Build request to send to server
char *http_build_request(http_request*);
// Send error reply to clients
char *http_build_reply();

// Utils

// Read a whole line from fd
int read_line_socket(int fd, char *dst, unsigned int size);

#endif