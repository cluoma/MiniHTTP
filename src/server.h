//
//  server.h
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-07-03.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#ifndef MiniHTTP_server_h
#define MiniHTTP_server_h

#include <sys/socket.h>

#include "request.h"

/* http_server structures stores information about the current server */
typedef struct
{
    // Config stuff
    char *port;
    int backlog;
    char *docroot;
    char *log_file;

    int daemon;

    // Sock stuff
    int sock;
} http_server;

/* Default http_server values for when arguments are missing */
static const http_server HTTP_SERVER_DEFAULT = {
    "3490",             // default port
    10,                 // default listen backlog
    "./docroot",        // default serving directory
    "./nubserv.log",    // default logfile
    0,                  // don't daemonize
    0                   // fd 0
};

/* Get network address structure */
void *get_in_addr(struct sockaddr *sa);

/* http server init and begin functions */
http_server http_server_new();
int http_server_start(http_server *server);
void http_server_run(http_server *server);

/* Write to server logfile */
void write_log(http_server *server, http_request *request, char *client_ip);

/* Reaps child processes from when requests are finished */
void sigchld_handler(int s);

#endif
