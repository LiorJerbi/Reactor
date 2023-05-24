#include <stdio.h>
#include "reactor.h"
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT "9034"   // Port we're listening on

int rListSize = 0,capacity = 10;
pthread_t reactor_thread;

void del_fd(void* this,int fd)
{
    Handler* rList = this;
    close(fd);
    for(int i=0;i<rListSize;i++){
        if(rList->pfds[i].fd == fd){
            rList->pfds[i] = rList->pfds[rListSize-1];
            rList->reactor[i] = rList->reactor[rListSize-1];
            rListSize--;
            break;
        }
    }
    // Copy the one from the end over this one

}




void* createReactor(){
    Handler* rList;
    rList = (Handler*)malloc(sizeof(Handler));
    if(rList == NULL){
        perror("malloc");
        exit(1);
    }
    rList->pfds = malloc(sizeof(struct pollfd)*capacity);
    rList->reactor = (Reactor*)malloc(sizeof(Reactor)*capacity);

    if(rList->pfds == NULL || rList->reactor == NULL){
        perror("malloc");
        exit(1);
    }
    return rList;
}

void addFd(void *this,int fd,void* func){
    Handler* rList = this;
    if(rList == NULL){
        perror("not initiallized");
        exit(1);
    }
    if(rListSize >= capacity){
        printf("stating realloc\n");
        capacity*=2;
        // rList = (Handler*)realloc(rList,sizeof(Handler) * capacity);
        rList->pfds = (struct pollfd*)realloc(rList->pfds,sizeof(struct pollfd)*capacity);
        rList->reactor = (Reactor*)realloc(rList->reactor,sizeof(Reactor)*capacity);
        if(rList == NULL || rList->pfds == NULL){
            perror("realloc");
            exit(1);
        }
    }
    rList->reactor[rListSize].fd = fd;
    rList->reactor[rListSize].fd_func = func;
    rList->pfds[rListSize].fd =fd;
    rList->pfds[rListSize].events = POLLIN;
    rListSize++;
}

void startReactor(void *this){
    pthread_create(&reactor_thread,NULL,hanldeReactor,this);
}

void* hanldeReactor(void* this) {
    Handler* rList;
    rList = (Handler*)this;
    
    for (;;) {
        int poll_count = poll(rList[0].pfds, rListSize, -1);
        if (poll_count == -1) {
            perror("poll");
            free(rList);
            exit(1);
        }
        // Run through the existing connections looking for data to read
        for (int i = 0; i < rListSize; i++) {
            // Check if someone's ready to read
            if (rList[0].pfds[i].revents & POLLIN) { // We got one!!
                if (i == 0) {
                    // fd is listener.
                    rList[0].reactor->fd_func(rList[i].pfds[i].fd, rList);
                }
                else {
                    rList[0].reactor[i].fd_func(rList[0].pfds[i].fd, rList);
                }
            }
        } // END handle data from client
    } // END got ready-to-read from poll()
}
void stopReactor(void *this){
    pthread_detach(reactor_thread);
}
void WaitFor(void* this){
    pthread_join(reactor_thread,NULL);
}
