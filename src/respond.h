//
//  respond.h
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-07-03.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#ifndef __MiniHTTP__respond__
#define __MiniHTTP__respond__

#include <stdio.h>

#define MAX(a,b) \
({ __typeof__ (a) _a = (a); \
__typeof__ (b) _b = (b); \
_a > _b ? _a : _b; })

void handle_request(int sock, http_request *request);

void send_file(int sock, FILE *f);

/* Get properly formatted url path from request */
char *url_path(http_request *request);

/* Decodes url hex codes to plaintext */
void html_to_text(const char *source, char *dest, size_t length);

char *sanitize_path(char *path);

#endif /* defined(__MiniHTTP__respond__) */
