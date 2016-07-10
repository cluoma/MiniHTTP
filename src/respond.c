//
//  respond.c
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-07-03.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <string.h>
#include <unistd.h>

#include "server.h"
#include "request.h"
#include "respond.h"
#include "cgibase.h"
#include "http_parser.h"
#include "mime_types.h"

void
handle_request(int sock, http_server *server, http_request *request)
{
    response_header rh;
    rh.status.version = "HTTP/1.1";
    
    switch (request->method) {
        case HTTP_GET:
        {
            char *url = url_path(request);
            char *file_path = malloc(strlen(server->docroot) + strlen(url) + 1);
            memset(file_path, 0, strlen(server->docroot) + strlen(url) + 1);
            file_path = strcat(file_path, server->docroot);
            file_path = strcat(file_path, url);
            free(url);
            
            file_stats fs = get_file_stats(file_path);
            build_header(&rh, &fs);
            
            if (fs.found)
            {
                if (strcasecmp(fs.extension, "cgi") == 0)
                {
                    exec_cgi(sock, request, file_path);
                } else {
                    send_header(sock, &rh, &fs);
                    send_file(sock, file_path, &fs);
                }
            } else
            {
                send(sock, "HTTP/1.1 404 Not Found\n", 23, 0);
            }
            free(file_path);
        }
            break;
        case HTTP_POST:
        {
            char *url = url_path(request);
            char *file_path = malloc(strlen(server->docroot) + strlen(url) + 1);
            memset(file_path, 0, strlen(server->docroot) + strlen(url) + 1);
            file_path = strcat(file_path, server->docroot);
            file_path = strcat(file_path, url);
            free(url);
            
            file_stats fs = get_file_stats(file_path);
            build_header(&rh, &fs);
            
            if (fs.found)
            {
                if (strcasecmp(fs.extension, "cgi") == 0)
                {
                    exec_cgi(sock, request, file_path);
                } else {
                    send_header(sock, &rh, &fs);
                    send_file(sock, file_path, &fs);
                }
            } else
            {
                send(sock, "HTTP/1.1 404 Not Found\n", 23, 0);
            }
            free(file_path);
        }
            break;
    }
}

void
send_header(int sock, response_header *rh, file_stats *fs)
{
    // Status line
    send(sock, rh->status.version, strlen(rh->status.version), 0);
    send(sock, " ", 1, 0);
    send(sock, rh->status.status_code, strlen(rh->status.status_code), 0);
    send(sock, " ", 1, 0);
    send(sock, rh->status.status, strlen(rh->status.status), 0);
    send(sock, "\r\n", 2, 0);
    
    // File content
    char *buf;
    asprintf(&buf, "Content-Type: %s\r\n", mime_from_ext(fs->extension));
    send(sock, buf, strlen(buf), 0);
    asprintf(&buf, "Content-Length: %lld\r\n", (long long int)fs->bytes);
    send(sock, buf, strlen(buf), 0);
    free(buf);
    
    // End of Header
    send(sock, "\r\n", 2, 0);
}

// Needs a lot of work
void
send_file(int sock, char *file_path, file_stats *fs)
{
    char *buf[256];
    
    FILE *f = fopen(file_path, "r");
    
    size_t n_read;
    while (!feof(f)) {
        n_read = fread(buf, 1, 256, f);
        send(sock, buf, n_read, 0);
    }
    
    fclose(f);
}

// Needs a lot of work
void
build_header(response_header *rh, file_stats *fs)
{
    if (fs->found)
    {
        rh->status.status_code = "200";
        rh->status.status = "OK";
    } else
    {
        rh->status.status_code = "404";
        rh->status.status = "Not Found";
    }
}

// Needs a lot of work
file_stats
get_file_stats(char *file_path)
{
    file_stats fs;
    struct stat s;
    
    if (stat(file_path, &s) == -1 ||
        !S_ISREG(s.st_mode) || S_ISDIR(s.st_mode)) {
        fs.found = 0;
    } else {
        fs.found = 1;
        fs.bytes = s.st_size;
        fs.extension = strrchr(file_path, '.') + 1; // +1 because of '.'
        fs.name = NULL;
    }
    
    return fs;
}

// Returns pointer to a string containing a sanitized url path
char *
url_path(http_request *request)
{
    if ((request->parser_url.field_set >> UF_PATH) & 1) {
        char *path = malloc(request->parser_url.field_data[UF_PATH].len + 1);
        html_to_text(request->uri+(request->parser_url.field_data[UF_PATH].off),
                     path,
                     request->parser_url.field_data[UF_PATH].len);
        path = sanitize_path(path);
        return path;
    } else { // Something went wrong, return base url path "/"
        char *path = malloc(2);
        path[0] = '/'; path[1] = '\0';
        return path;
    }
}

// Decodes URL strings to text (eg '+' -> ' ' and % hex codes)
void
html_to_text(const char *source, char *dest, size_t length)
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
char *
sanitize_path(char *path)
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