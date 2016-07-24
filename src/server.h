//
//  server.h
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-07-03.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#ifndef MiniHTTP_server_h
#define MiniHTTP_server_h

#include "request.h"

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

static const http_server HTTP_SERVER_DEFAULT = {
    "3490",             // default port
    10,                 // default listen backlog
    "./docroot",        // default serving directory
    "./nubserv.log",    // default logfile
    0,                  // don't daemonize
    0                   // fd 0
};

void *get_in_addr(struct sockaddr *sa);

http_server http_server_new();
int http_server_start(http_server *server);
void http_server_run(http_server *server);

void write_log(http_server *server, http_request *request);

/*
 * Reaps child processes from when requests are finished
 */
void sigchld_handler(int s);

//void parse_args(int argc, char **argv, http_server *server);

#endif
