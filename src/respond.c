//
//  respond.c
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-07-03.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#ifdef __linux__
#include <sys/sendfile.h>
#endif
#include <string.h>
#include <unistd.h>
#include <errno.h>

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
        case HTTP_POST:
        case HTTP_GET:
        {
            char *url = url_path(request);
            if (strcmp(url, "") == 0)
            {
                url = realloc(url, strlen("/cgi-bin/bb.cgi")+1);
                memset(url, 0, strlen("/cgi-bin/bb.cgi")+1);
                strcpy(url, "/cgi-bin/bb.cgi");
            }

            char *file_path = calloc(strlen(server->docroot) + strlen(url) + 1, 1);
            //memset(file_path, 0, strlen(server->docroot) + strlen(url) + 1);
            file_path = strcat(file_path, server->docroot);
            file_path = strcat(file_path, url);
            free(url);

            file_stats fs = get_file_stats(file_path);

            // File found and it's a directory, look for default file
            if (fs.found && fs.isdir) {
                file_path = realloc(file_path, strlen(file_path) + strlen(server->default_file) + 2);
                file_path = strcat(file_path, "/");
                file_path = strcat(file_path, server->default_file);
                fs = get_file_stats(file_path);
            }

            if (fs.found && !fs.isdir)
            {
                if (strcasecmp(fs.extension, "cgi") == 0)
                {
                    request->keep_alive = HTTP_CLOSE;
                    exec_cgi(sock, request, file_path);
                }
                // else if (strcasecmp(fs.extension, "php") == 0)
                // {
                //     request->keep_alive = HTTP_CLOSE;
                //     exec_php(sock, request, file_path);
                // }
                else
                {
                    // Add file information to header
                    build_header(&rh, &fs);
                    send_header(sock, request, &rh, &fs);
                    send_file(sock, file_path, &fs, server->use_sendfile);
                }
            } else
            {
		        char resp_not_found[300];
		        char *not_found = "<html><p>404 Not Found</p></html>";
		        sprintf(resp_not_found, "HTTP/1.1 404 Not Found\r\nServer: minihttp\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n%s", (int)strlen(not_found), not_found);
                //send(sock, "HTTP/1.1 404 Not Found\r\n", 24, 0);
                //send(sock, "Content-Length: 0\r\n\r\n", 21, 0);
		        send(sock, resp_not_found, strlen(resp_not_found), 0);
            }
            free(file_path);
        }
        break;
    }
}

void
send_header(int sock, http_request *request, response_header *rh, file_stats *fs)
{
    /* TODO:
     * make this cleaner
     */
    char headers[1024];
    // Status line
//    send(sock, rh->status.version, strlen(rh->status.version), 0);
//    send(sock, " ", 1, 0);
//    send(sock, rh->status.status_code, strlen(rh->status.status_code), 0);
//    send(sock, " ", 1, 0);
//    send(sock, rh->status.status, strlen(rh->status.status), 0);
//    send(sock, "\r\n", 2, 0);

    // Server info
//    send(sock, "Server: minihttp\r\n", 18, 0);

    sprintf(headers, "%s %s %s\r\nServer: minihttp\r\n", rh->status.version, rh->status.status_code, rh->status.status);

//    char *buf;
//    int bytes;
    // Keep Alive
    if (request->keep_alive == HTTP_KEEP_ALIVE)
    {
//        send(sock, "Connection: Keep-Alive\r\n", 24, 0);
//        send(sock, "Keep-Alive: timeout=5\r\n", 23, 0);
        sprintf(headers, "%sConnection: Keep-Alive\r\nKeep-Alive: timeout=5\r\n", headers);
    }
    else
    {
//        send(sock, "Connection: Close\r\n", 19, 0);
        sprintf(headers, "%sConnection: Close\r\n", headers);
    }
    // File content
//    bytes = asprintf(&buf, "Content-Type: %s\r\n", mime_from_ext(fs->extension));
//    send(sock, buf, bytes, 0);
//    free(buf);
//    bytes = asprintf(&buf, "Content-Length: %lld\r\n", (long long int)fs->bytes);
//    send(sock, buf, bytes, 0);
//    free(buf);
    sprintf(headers, "%sContent-Type: %s\r\nContent-Length: %lld\r\n\r\n", headers, mime_from_ext(fs->extension), (long long int)fs->bytes);
    send(sock, headers, strlen(headers), 0);
    // End of Header
//    send(sock, "\r\n", 2, 0);
}

// Needs a lot of work
void
send_file(int sock, char *file_path, file_stats *fs, int use_sendfile)
{
    if (use_sendfile)
    {
        int f = open(file_path, O_RDONLY);
        if ( f <= 0 )
        {
            printf("Cannot open file %d\n", errno);
            return;
        }

        off_t len = 0;
        #ifdef __APPLE__
        if ( sendfile(f, sock, 0, &len, NULL, 0) < 0 )
        {
            printf("Mac: Sendfile error: %d\n", errno);
        }
        #elif __linux__
        size_t sent = 0;
        ssize_t ret;
        while ( (ret = sendfile(sock, f, &len, fs->bytes - sent)) > 0 )
        {
            sent += ret;
            if (sent >= fs->bytes) break;
        }
        #endif
        close(f);
    }
    else
    {
        FILE *f = fopen(file_path, "rb");
        if ( f == NULL )
        {
            printf("Cannot open file %d\n", errno);
            return;
        }

        size_t len = 0;
        char *buf = malloc(TRANSFER_BUFFER);
        while ( (len = fread(buf, 1, TRANSFER_BUFFER, f)) > 0 )
        {
            ssize_t sent = 0;
            ssize_t ret  = 0;
            while ( (ret = send(sock, buf+sent, len-sent, 0)) > 0 )
            {
                sent += ret;
                if (sent >= fs->bytes) break;
            }
            if (ret < 0)
            {
                printf("ERROR!!!\n");
                break;
            }

            // Check for being done, either fread error or eof
            if (feof(f) || ferror(f)) {break;}
        }
        free(buf);
        fclose(f);
    }

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

    if (stat(file_path, &s) == -1) {  // Error in stat
        fs.found = 0;
        fs.isdir = 0;
    } else if (S_ISDIR(s.st_mode)) {  // Found a directory
        fs.found = 1;
        fs.isdir = 1;
    } else if (S_ISREG(s.st_mode)) {  // Found a file
        fs.found = 1;
        fs.isdir = 0;
        fs.bytes = s.st_size;
        fs.extension = strrchr(file_path, '.') + 1; // +1 because of '.'
        fs.name = NULL;
    } else {  // Anything else we pretend we didn't find it
        fs.found = 0;
        fs.isdir = 0;
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
