#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dlfcn.h>
#include "reactor.h"

#define PORT "9034"   // Port we're listening on
void (*func)(int);

// bool init_library(char *arg)
// {
//     void *hdl;
//     hdl = dlopen("./st_reactor.so",RTLD_LAZY);
//     if (NULL == hdl){
//         return false;
//     }
//     func = (void(*)(int))dlsym(hdl,arg);
//     if (NULL == func)
//     {
//         return false;
//     }
        
    
// }
// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Return a listening socket
int get_listener_socket(void)
{
    int listener;     // Listening socket descriptor
    int yes=1;        // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai); // All done with this

    // If we got here, it means we didn't get bound
    if (p == NULL) {
        return -1;
    }

    // Listen
    if (listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
}
void getConnection(int fd, void* reactor){
    struct sockaddr_storage remoteaddr; // Client address
    socklen_t addrlen;
    addrlen = sizeof remoteaddr;
    char remoteIP[INET6_ADDRSTRLEN];
    int newfd = accept(fd,
        (struct sockaddr *)&remoteaddr,
        &addrlen);
    if (newfd == -1) {
        perror("accept");
    }   
    printf("pollserver: new connection from %s on "
    "socket %d\n",
    inet_ntop(remoteaddr.ss_family,
        get_in_addr((struct sockaddr*)&remoteaddr),
        remoteIP, INET6_ADDRSTRLEN),
    newfd);
    addFd(reactor,newfd,getData);
    
}

// Main
int main(void)
{

    Handler* handle = createReactor();

    // Reactor reactor;
    // reactor.fd_func = getConnection;
    int listener;     // Listening socket descriptor


    // Set up and get a listening socket
    listener = get_listener_socket();
    if (listener == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    addFd(handle,listener,getConnection);
    startReactor(handle);
    WaitFor(handle);

    return 0;
}