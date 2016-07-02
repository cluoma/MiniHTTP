//
//  request.h
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-06-27.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#ifndef __MiniHTTP__request__
#define __MiniHTTP__request__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "http_parser.h"

typedef struct http_request http_request;

struct http_request {
    // Request buffer
    char *request;
    size_t request_len;
    
    // First line
    int method;
    const char *uri;
    size_t uri_len;
    const char *version;
    size_t version_len;
    
    size_t content_length;
    size_t header_length;
    
    // Headers
    size_t header_fields;
    size_t header_values;
    const char **header_field;
    size_t *header_field_len;
    const char **header_value;
    size_t *header_value_len;
    
    // Body
    const char *body;
    size_t body_len;
};

/* Request parsing callback functions
 * all callbacks return 0 on succes, -1 otherwise
 */
int start_cb(http_parser* parser);
int url_cb(http_parser* parser, const char *at, size_t length);
int header_field_cb(http_parser* parser, const char *at, size_t length);
int header_value_cb(http_parser* parser, const char *at, size_t length);
int header_end_cb(http_parser* parser);
int body_cb(http_parser* parser, const char *at, size_t length);

/* Free memory used by http_request */
void init_request(http_request *request);
void free_request(http_request *request);

//void recieve_data(int sock, http_request *request);
void receive_data(int sock, http_parser *parser);
ssize_t read_chunk(int sock, char **str, ssize_t t_recvd, size_t chunk_size);


#endif /* defined(__MiniHTTP__request__) */
