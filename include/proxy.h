#ifndef PROXY_H
#define PROXY_H

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include "constants.h"
#include "http.h"

// Signal handling
void SIGUSR1_handler(int signum);
void SIGUSR2_handler(int signum);
// Network functions
int connect_server(http_request *);
int send_request(int, char*);
int receive_and_reply_content(int server_fd, int client_fd);
int send_line(int client_fd, char*line);
int send_all_to_socket(int sock_fd, char *buf, int buf_len, int *send_len);
// List functions
const char *list_get_value(struct METADATA_HEAD *, const char *);

#endif