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

Handler* rList = NULL;
int rListSize = 0,capacity = 10;
pthread_t reactor_thread;

void del_fd(int fd)
{
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
void getData(int fd,void* reactor){
    char buf[1024];
    int nbytes = recv(fd, buf, sizeof(buf), 0);
    if (nbytes <= 0) {
        // Got error or connection closed by client
        if (nbytes == 0) {
            // Connection closed
            printf("pollserver: socket %d hung up\n", fd);
        } else {
            perror("recv");
        }
        del_fd(fd);
    } else {
        // We got some good data from a client
        printf("%s\n",buf);
    }
}



void* createReactor(){
    rList = (Handler*)malloc(sizeof(Handler)*capacity);
    if(rList == NULL){
        perror("malloc");
        exit(1);
    }
    return rList;
}

void addFd(void *this,int fd,st_f_reactor func){
    if(rList == NULL){
        perror("not initiallized");
        exit(1);
    }
    if(rListSize >= capacity){
        capacity*=2;
        rList = (Handler*)realloc(rList,sizeof(Handler) * capacity);
        if(rList == NULL){
            perror("realloc");
            exit(1);
        }
    }
    rList[rListSize].reactor->fd=fd;
    rList[rListSize].reactor->fd_func=func;
    rList[rListSize].pfds[rListSize].fd =fd;
    rList[rListSize].pfds[rListSize].events = POLLIN;
    rListSize++;
}

void startReactor(void *this){
    pthread_create(&reactor_thread,NULL,hanldeReactor,NULL);
}

void* hanldeReactor(){
    for(;;) {
        int poll_count = poll(rList->pfds,rListSize,-1);
        if (poll_count == -1) {
            perror("poll");
            free(rList);
            exit(1);
        }
        // Run through the existing connections looking for data to read
        for(int i = 0; i < rListSize; i++) {
            // Check if someone's ready to read
            if (rList->pfds[i].revents & POLLIN) { // We got one!!
                if (i == 0) {
                    // fd is listener.
                    rList[0].reactor->fd_func(rList[0].pfds->fd,rList);
                }
                else {
                    rList[i].reactor->fd_func(rList[i].pfds->fd,rList);
                } 
            }
        } // END handle data from client
    } // END got ready-to-read from poll()
} // END looping through file descriptors

void stopReactor(void *this){
    pthread_detach(reactor_thread);
}
void WaitFor(void* this){
    pthread_join(reactor_thread,NULL);
}
