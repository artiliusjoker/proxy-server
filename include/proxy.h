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

#include "constants.h"
#include "http.h"

// Signal handling
void SIGUSR1_handler(int signum);
void SIGUSR2_handler(int signum);
void SIGINT_handler(int signum);

#endif