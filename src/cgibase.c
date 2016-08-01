//
//  cgibase.c
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-07-07.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "cgibase.h"
#include "request.h"

// Forks process to exec a cgi
void
exec_cgi(int sock, http_request *request, char *file_path)
{
    // Setup environ variables
    char *envp[5];
    asprintf(&envp[0], "REQUEST_METHOD=%s", http_method_str(request->method));
    asprintf(&envp[1], "CONTENT_LENGTH=%ld", (long int)request->content_length);
    envp[3] = NULL;
    
    char *tmp;
    if ((request->parser_url.field_set >> UF_QUERY) & 1)
    {
        tmp = malloc(request->parser_url.field_data[UF_QUERY].len + 1);
        strncpy(tmp,
                request->uri+(request->parser_url.field_data[UF_QUERY].off),
                request->parser_url.field_data[UF_QUERY].len);
        tmp[request->parser_url.field_data[UF_QUERY].len] = '\0';
    } else
    {
        tmp = malloc(1); tmp[0] = '\0';
    }
    asprintf(&envp[2], "QUERY_STRING=%s", tmp);
    free(tmp);
    
    int pipefd[2];
    pipe(pipefd);
    
    //printf("FILE: %s\n", file_path);
    write(pipefd[1], request->body, request->content_length);
    
    int c_pid = fork();
    if (c_pid == 0)
    { // Child
        close(pipefd[1]);
        send(sock, "HTTP/1.1 200 OK\r\n", 17, 0);
        //send(sock, "Content-Length: 17926\r\n", 23, 0);
        dup2(sock, STDOUT_FILENO);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execle(file_path, strrchr(file_path, '/')+1, (char *)NULL, envp);
    } else if (c_pid == -1) { // Couldnt fork
        
        send(sock, "HTTP/1.1 404 Not Found\r\n", 17, 0);
    }
    else
    { // Parent
        close(pipefd[0]);
        
        // Free envp while waiting
        for (char **env = envp; (*env) != NULL; env++)
        {
            free((*env));
        }
        
        // Wait for child to finish
        waitpid(c_pid, NULL, 0);
        
        close(pipefd[1]);
    }
}
