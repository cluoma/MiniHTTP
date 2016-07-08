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

#include "server.h"


void parse_args(int argc, char **argv, http_server *server)
{
    int c;
    while ((c = getopt(argc, argv, "p:d:b:a:l:")) != -1) {
        switch (c) {
            case 'p':
                server->port = optarg;
                break;
            case 'd':
                server->docroot = optarg;
                break;
            case 'b':
                server->backlog = atoi(optarg);
                break;
            case 'a':
                server->daemon = 1;
                break;
            case 'l':
                server->log = 1;
                break;
            case '?':
                if (optopt == 'c' || optopt == 'd' || optopt == 'b')
                {
                    fprintf(stderr, "Error: -%c option missing\n", optopt);
                    exit(1);
                }
                else
                {
                    fprintf(stderr, "Error: -%c unknown option\n", optopt);
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, "Error parsing options\nExiting...");
                exit(1);
        }
    }
    
}

// Reap zombie processes
void sigchld_handler(int s)
{
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

int main(int argc, char **argv)
{
    // Init server with arguments
    http_server server = HTTP_SERVER_DEFAULT;
    parse_args(argc, argv, &server);
    
    // Turn into daemon if selected
    if (server.daemon) daemon(1, 1);
    
    
    printf("Init with: port:%s backlog:%d docroot:%s\n", server.port, server.backlog, server.docroot);
    
    // setup SIGCHLD signal handling
    struct sigaction sa;
    
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    server_init(&server);
    server_main_loop(&server);
    
    return 0;
}
