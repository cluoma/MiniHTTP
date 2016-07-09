//
//  server.h
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-07-03.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#ifndef MiniHTTP_server_h
#define MiniHTTP_server_h

typedef struct http_server http_server;
struct http_server
{
    // Config stuff
    char *port;
    int backlog;
    char *docroot;
    char *log_file;
    int daemon;
    
    // Sock stuff
    int sock;
};
static const http_server HTTP_SERVER_DEFAULT = {
    "3490", 10, "./docroot", "./", 0, 0
};

void *get_in_addr(struct sockaddr *sa);

void server_init(http_server *server);

void server_main_loop(http_server *server);

void parse_args(int argc, char **argv, http_server *server);

#endif
