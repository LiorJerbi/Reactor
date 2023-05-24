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
void (*addfd)(void* ,int,void*);
void (*start)(void*);
void (*wait)(void *);
void (*stop)(void *);
void (*del)(void *,int);
void* (*create)();


void getData(int fd,void* reactor){
    char buf[1024];
    memset(buf,0,sizeof(buf));
    int nbytes = recv(fd, buf, sizeof(buf), 0);
    if (nbytes <= 0) {
        if (nbytes == 0) {
            printf("pollserver: socket %d hung up\n", fd);
        } else {
            perror("recv");
        }
        del(reactor,fd);
    } else {
        printf("%s",buf);
    }
}

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
    printf("starting connection\n");
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
        remoteIP, INET6_ADDRSTRLEN),newfd);
    addfd(reactor,newfd,&getData);
    
}

// Main
int main(void)
{
    void *handler_lib = dlopen("./libst_reactor.so", RTLD_LAZY);
    if (!handler_lib) {
        fprintf(stderr, "Error loading event handler library: %s\n", dlerror());
        return 1;
    }

    
    addfd = dlsym(handler_lib, "addFd");
    start = dlsym(handler_lib, "startReactor");
    wait = dlsym(handler_lib, "WaitFor");
    stop = dlsym(handler_lib, "stopReactor");
    create = dlsym(handler_lib, "createReactor");
    del = dlsym(handler_lib, "del_fd");

    if (!addfd || !start || !wait || !stop || !create || !del) {
        fprintf(stderr, "Error loading function pointers: %s\n", dlerror());
        return 1;
    }

    Handler* handle = create();
    int listener;     
    listener = get_listener_socket();
    if (listener == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    addfd(handle,listener,&getConnection);
    start(handle);
    wait(handle);

    return 0;
}