//
//  server.c
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-07-05.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>

#include "server.h"
#include "request.h"
#include "respond.h"
#include "http_parser.h"

// get sockaddr, IPv4 or IPv6:
void *
get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//
void
write_log(http_server *server, http_request *request)
{
    FILE *f = fopen(server->log_file, "a"); // open for writing
    if (f == NULL) return;
    
    // Get current time
    time_t timer;
    char buffer[26];
    struct tm* tm_info;
    time(&timer);
    tm_info = gmtime(&timer);
    strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
    
    // Log method
    fwrite(http_method_str(request->method), 1, strlen(http_method_str(request->method)), f);
    fwrite(",", 1, 1, f);
    
    // Log URI
    if (strcmp(http_method_str(request->method), "<unknown>") == 0)
    {
        fwrite(",", 1, 1, f);
    } else
    {
        fwrite(request->uri, 1, request->uri_len, f);
        fwrite(",", 1, 1, f);
    }
    
    // Log GMT timestamp
    fwrite(buffer, 1, strlen(buffer), f);
    fwrite("\n", 1, 1, f);
    
    fclose(f);
}

void
server_init(http_server *server)
{
    struct addrinfo hints, *servinfo, *p;
    
    int rv;
    int yes = 1;
    
    // Fill hints, all unused elements must be 0 or null
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // ipv4/6 don't care
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    
    // Get info for us
    if ((rv = getaddrinfo(NULL, server->port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((server->sock = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(server->sock, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("server: setsockopt");
            exit(1);
        }
        if (bind(server->sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(server->sock);
            perror("server: bind");
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo); // all done with this structure
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    if (listen(server->sock, server->backlog) == -1) {
        perror("listen");
        exit(1);
    }
}

void
server_main_loop(http_server *server)
{
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    
    int conn_fd;
    
    printf("server: waiting for connections...\n");
    while(1) {
        sin_size = sizeof their_addr;
        conn_fd = accept(server->sock, (struct sockaddr *)&their_addr, &sin_size);
        if (conn_fd == -1) {
            perror("accept");
            continue;
        }
        
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);
        
        
        
        if (!fork()) { // this is the child process
            close(server->sock); // child doesn't need the listener
            
            // init http parser
            http_parser *parser = malloc(sizeof(http_parser));
            http_parser_init(parser, HTTP_REQUEST);
            
            // Create request data
            http_request request;
            init_request(&request);
            parser->data = &request;
            
            // read request data from client
            receive_data(conn_fd, parser);
            
            write_log(server, &request);
            
//            printf("FIELDS: %d VALUES: %d\n", (int)request.header_fields, (int)request.header_values);
//                        for (int i = 0; i < request.header_values; i++) {
//                            printf("%d ", i);
//                            printf("HEADER FIELD: %.*s ", (int)request.header_field_len[i], request.header_field[i]);
//                            printf("%.*s\n", (int)request.header_value_len[i], request.header_value[i]);
//                            printf("BODYY: %d\n", (int)request.body_len);
//                        }
//            printf("METHOD: %d %s\n", parser->method, http_method_str(parser->method));
            
            handle_request(conn_fd, server, &request);
            
            // Cleanup
            free_request(&request);
            free(parser);
            close(conn_fd);
            exit(0);
        }
        close(conn_fd);  // parent doesn't need this
    }
}
