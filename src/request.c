//
//  request.c
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-06-27.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include "request.h"

// Handle incoming request data
void recieve_data(int sock, http_request *request)
{
    int is_eoh = 0;
    size_t content_length = 0;
    size_t header_length = 0;
    
    size_t buf_size = 512;
    
    char *str = malloc(buf_size);
    if (str == NULL && errno == ENOMEM) {
        goto bad;
    }
    
    ssize_t t_recvd = 0;
    ssize_t n_recvd;
    while((n_recvd = recv(sock, str+t_recvd, buf_size, 0)) >= -1) {
        if (n_recvd == 0) { // Connection closed by client
            perror("reading");
            break;
        } else if (n_recvd == -1) { // Error receiving
            perror("reading");
            break;
        } else {
            t_recvd += n_recvd;
            str = realloc(str, t_recvd + buf_size);
            if (str == NULL && errno == ENOMEM) {
                goto bad;
            }
        }
        
        // Break out
        if (!is_eoh) {
            if (strnstr(str, "\r\n\r\n", t_recvd) != NULL) {
                is_eoh = 1;
                content_length = something(str, t_recvd);
                break;
            }
        } else if (is_eoh && t_recvd <= header_length + content_length) {
            
        }
    }
    request->request = str;
    return;
bad:
    perror("receive data");
    return;
}

int something(char *str, size_t length) {
    char *content_length = strnstr(str, "Content-Length:", length);
    if (content_length == NULL) {
        return -1;
    } else {
        printf("CONTENT LENGTH: %d\n", atoi(content_length+strlen("Content-Length:")));
        return atoi(str+strlen("Content-Length: "));
    }
}


/*
 * Parsing callbacks
 */
int start_cb(http_parser* parser)
{
    http_request *request = parser->data;
    
    request->content_length = 0;
    
    request->header_fields = 0;
    request->header_values = 0;
    request->header_field = NULL;
    request->header_field_len = NULL;
    request->header_value = NULL;
    request->header_value_len = NULL;
    
    return 0;
}

int url_cb(http_parser* parser, const char *at, size_t length)
{
    http_request *request = parser->data;
    
    request->uri = at;
    request->uri_len = length;
    
    return 0;
}

int header_field_cb(http_parser* parser, const char *at, size_t length)
{
    http_request *request = parser->data;
    
    request->header_field = realloc(request->header_field, sizeof(char*)*(request->header_fields + 1));
    request->header_field_len = realloc(request->header_field_len, sizeof(size_t)*(request->header_fields + 1));
    request->header_field[request->header_fields] = at;
    request->header_field_len[request->header_fields] = length;
    
    request->header_fields += 1;
    
    return 0;
}

int header_value_cb(http_parser* parser, const char *at, size_t length)
{
    http_request *request = parser->data;
    
    request->header_value = realloc(request->header_value, sizeof(char*)*(request->header_values + 1));
    request->header_value_len = realloc(request->header_value_len, sizeof(size_t)*(request->header_values + 1));
    request->header_value[request->header_values] = at;
    request->header_value_len[request->header_values] = length;
    
    request->header_values += 1;
    
    return 0;
}
