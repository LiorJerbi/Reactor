#ifndef _REACTOR_H
#define _REACTOR_H


typedef void (*st_f_reactor)(int fd, void* reactor);
typedef struct _Reactor{
    int fd;
    st_f_reactor fd_func;
}Reactor,*pReactor;

typedef struct _Handler{
    Reactor* reactor;
    struct pollfd *pfds;
}Handler;

void* createReactor();
void stopReactor(void *this);
void startReactor(void *this);
void addFd(void *this,int fd,st_f_reactor hlist);
void WaitFor(void* this);
void* hanldeReactor();
void getConnection(int fd,void *reactor);
void getData(int fd,void *reactor);
void del_fd(int fd);

#endif