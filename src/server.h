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
    char *port;
    int backlog;
    char *docroot;
};
const http_server HTTP_SERVER_DEFAULT = {
    "3490", 10, "./docroot"
};

void parse_args(int argc, char **argv, http_server server);

#endif
