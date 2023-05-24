#ifndef _REACTOR_H
#define _REACTOR_H


typedef void* (*gg)(int fd, void* reactor);


typedef struct _Reactor{
    int fd;
    gg fd_func;
}Reactor,*pReactor;

typedef struct _Handler{
    Reactor* reactor;
    struct pollfd *pfds;
}Handler;

void* createReactor();
void stopReactor(void*);
void startReactor(void*);
void addFd(void*,int,void*);
void WaitFor(void*);
void* hanldeReactor(void* );

void del_fd(void*,int);

#endif