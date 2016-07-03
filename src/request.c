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


void receive_data(int sock, http_parser *parser)
{
    http_request *request = parser->data;
    init_request(request);
    
    // Init http parser settings
    http_parser_settings settings;
    http_parser_settings_init(&settings);
    settings.on_message_begin = start_cb;
    settings.on_url = url_cb;
    settings.on_header_field = header_field_cb;
    settings.on_header_value = header_value_cb;
    settings.on_headers_complete = header_end_cb;
    settings.on_body = body_cb;
    
    size_t header_length = 0;
    
    size_t buf_size = 10;
    
    char *str = malloc(buf_size+1);
    if (str == NULL && errno == ENOMEM) {
        goto bad;
    }
    
    ssize_t t_recvd = 0;
    ssize_t n_recvd;
    while((n_recvd = recv(sock, str+t_recvd, buf_size, 0)) > 0) {
        // Add data to buffer
        t_recvd += n_recvd;
        str[t_recvd] = '\0';
        str = realloc(str, t_recvd + buf_size + 1);
        if (str == NULL && errno == ENOMEM) {
            goto bad;
        }
        
        // Got end of headers, break out
        char *tmp;
        if ((tmp = strstr(str, "\r\n\r\n")) != NULL) {
            header_length = tmp - str + 4;
            request->header_length = header_length;
            printf("HEADER LENGTH: %d\n", (int)header_length);
            http_parser_execute(parser, &settings, str, header_length);
            break;
        }
    }
    // Do we need more data based on content-length?
    while (t_recvd < request->content_length + header_length &&
           (n_recvd = read_chunk(sock, &str, t_recvd, buf_size)) > 0) {
            t_recvd += n_recvd;
    }
    
    printf("REQUEST:\n %.*s", (int)t_recvd, str);
    
    // Something went wrong
    if (n_recvd == 0 || n_recvd == -1) { // Connection closed by client or recv error
        goto bad;
    }
    
    // Parse the rest of the input
    http_parser_execute(parser, &settings, str+header_length, t_recvd-header_length);
    
    request->request = str;
    request->request_len = t_recvd;
    printf("TOTAL RECEIVED: %d\n", (int)request->request_len);
    return;
    
bad:
    if (str != NULL)
        free(str);
    free_request(request);
    perror("receive data");
    return;
}

// Reads maximum of 'chunk_size' bytes from socket into str
// Returns actual number of bytes read
ssize_t read_chunk(int sock, char **str, ssize_t t_recvd, size_t chunk_size)
{
    char *tmp = (*str);
    ssize_t n_recvd = recv(sock, tmp+t_recvd, chunk_size, 0);
    
    if (n_recvd == 0 || n_recvd == -1) { // recv error
        return n_recvd;
    }
    
    tmp = realloc(tmp, t_recvd + n_recvd + chunk_size + 1);
    if (tmp == NULL && errno == ENOMEM) { // realloc error
        return -1;
    }
    
    tmp[t_recvd+n_recvd] = '\0';
    (*str) = tmp;
    return n_recvd;
}

void init_request(http_request *request)
{
    request->content_length = 0;
    
    request->header_fields = 0;
    request->header_values = 0;
    request->header_field = NULL;
    request->header_field_len = NULL;
    request->header_value = NULL;
    request->header_value_len = NULL;
}

void free_request(http_request *request)
{
    if (request->request != NULL)
        free(request->request);
    if (request->header_field != NULL)
        free(request->header_field);
    if (request->header_field_len != NULL)
        free(request->header_field_len);
    if (request->header_value != NULL)
        free(request->header_value);
    if (request->header_value_len != NULL)
        free(request->header_value_len);
}


/*
 * Parsing callbacks
 */
int start_cb(http_parser* parser)
{
    printf("STARTED PARSING\n");
    return 0;
}

int url_cb(http_parser* parser, const char *at, size_t length)
{
    http_request *request = parser->data;
    
    request->uri = at;
    request->uri_len = length;
    
    http_parser_parse_url(at, length, 1, &(request->parser_url));
    
    struct http_parser_url parsed_url = request->parser_url;
    if ((parsed_url.field_set >> UF_PATH) & 1)
        printf("URL: %.*s\n", parsed_url.field_data[UF_PATH].len, at+parsed_url.field_data[UF_PATH].off);
    
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

int header_end_cb(http_parser* parser)
{
    http_request *request = parser->data;
    
    // Get content length
    request->content_length = parser->content_length;
    // Get http method
    request->method = parser->method;
    
    //printf("HEADER END\n");
    return 0;
}

int body_cb(http_parser* parser, const char *at, size_t length)
{
    http_request *request = parser->data;
    
    request->body = at;
    request->body_len = length;
    
    //printf("PARSED BODY\n");
    return 0;
}
