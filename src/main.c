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
#include "http_parser.h"


#define MYPORT "3490"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold
#define DOCROOT "/Users/colin/Documents/MiniHTTP/MiniHTTP/docroot"

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int url_callback(http_parser* parser, const char *at, size_t length){
    printf("URL: %s\n", at);
    printf("URL: %.*s\n", length, at);
    return 0;
}

//int request_handler(int sock)
//{
//    char buf[1000];
//    size_t c_recv = recv(sock, buf, 999, 0);
//    buf[c_recv] = '\0';
//    printf("server: request: %s\n", buf);
//    
//    //if (!fork()) {
//
//        size_t nparsed = http_parser_execute(parser, &settings, buf, c_recv);
//        printf("Bytes parsed: %d\n", (int)nparsed);
//    
//    //}
//    
//    if (send(sock, "Hello, world!", 13, 0) == -1)
//        perror("send");
//    return 0;
//}

int main(void)
{
    int sockfd = 0; // listen on sockfd, new connection on new_fd
    int new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    
    struct sigaction sa;
    int yes = 1;
    
    char s[INET6_ADDRSTRLEN];
    int rv;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // ipv4/6 don't care
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    
    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
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
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    
    
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    http_parser_settings settings;
    settings.on_url = url_callback;
    http_parser *parser = malloc(sizeof(http_parser));
    http_parser_init(parser, HTTP_REQUEST);
    
    printf("server: waiting for connections...\n");
    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);
        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            //request_handler(new_fd);
            
            char buf[1000];
            size_t c_recv = recv(new_fd, buf, 999, 0);
            buf[c_recv] = '\0';
            printf("server: request: %s\n", buf);
            
            //if (!fork()) {
            
            size_t nparsed = http_parser_execute(parser, &settings, buf, c_recv);
            printf("Bytes parsed: %d\n", (int)nparsed);
            
            //}
            
            if (send(new_fd, "Hello, world!", 13, 0) == -1)
                perror("send");
            
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }
    return 0;
}