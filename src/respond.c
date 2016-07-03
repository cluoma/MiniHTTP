//
//  respond.c
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-07-03.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#include <sys/socket.h>

#include "request.h"
#include "respond.h"
#include "http_parser.h"

#define DOCROOT "/Users/colin/Documents/MiniHTTP/MiniHTTP/docroot"

void handle_request(int sock, http_request *request)
{
    switch (request->method) {
        case 1: // GET
        {
            char *url = url_path(request);
            char *file_path = malloc(strlen(DOCROOT) + strlen(url) + 1);
            memset(file_path, 0, strlen(DOCROOT) + strlen(url) + 1);
            file_path = strcat(file_path, DOCROOT);
            file_path = strcat(file_path, url);
            free(url);
            
            FILE *f = fopen(file_path, "r");
            
            send(sock, "HTTP/1.1 200 OK\n", 16, 0);
            send(sock, "Context-Type: text/html\n\n", 25, 0);

            send_file(sock, f);
            
            fclose(f);
            free(file_path);
        }
    }
}

void send_file(int sock, FILE *f)
{
    char *buf[256];
    
    size_t n_read;
    while (!feof(f)) {
        n_read = fread(buf, 1, 256, f);
        send(sock, buf, n_read, 0);
    }
}

// Returns pointer to a string containing a sanitized url path
char *url_path(http_request *request)
{
    if ((request->parser_url.field_set >> UF_PATH) & 1) {
        char *path = malloc(request->parser_url.field_data[UF_PATH].len + 1);
        html_to_text(request->uri+(request->parser_url.field_data[UF_PATH].off),
                     path,
                     request->parser_url.field_data[UF_PATH].len);
        printf("PARSED URL: %s\n", path);
        path = sanitize_path(path);
        return path;
    } else {
        return "/";
    }
}

// Decodes URL strings to text (eg '+' -> ' ' and % hex codes)
void html_to_text(const char *source, char *dest, size_t length)
{
    size_t n = 0;
    while (*source != '\0' && n < length ) {
        if (*source == '+') {
            *dest = ' ';
        }
        else if (*source == '%') {
            int hex_char;
            sscanf(source+1, "%2x", &hex_char);
            *dest = hex_char;
            source += 2;
            n += 2;
        } else {
            *dest = *source;
        }
        source++;
        dest++;
        n++;
    }
    *dest = '\0';
}

// Removes './' and '/../' from paths
char *sanitize_path(char *path)
{
    char *token, *tofree;
    tofree = path;
    
    char *clean = malloc(strlen(path) + 1);
    memset(clean, 0, strlen(path));
    
    char **argv = malloc(sizeof(char *));
    int argc = 0;
    
    // Tokenize
    while ((token = strsep(&path, "/")) != NULL) {
        if (strcmp(token, ".") == 0 || strcmp(token, "") == 0) {
            continue;
        } else if (strcmp(token, "..") == 0) {
            argc = MAX(--argc, 0);
        } else {
            argv[argc] = token;
            argc++;
            argv = realloc(argv, sizeof(char *) * (argc + 1));
        }
    }
    
    // Combine cleaned filepath
    for (int i = 0; i < argc; i++) {
        strcat(clean, "/");
        strcat(clean, argv[i]);
    }
    
    free(tofree);
    free(argv);
    
    return clean;
}