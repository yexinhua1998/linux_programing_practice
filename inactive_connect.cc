#include<fcntl.h>
#include<sys/epoll.h>
#include<signal.h>
#include<errno.h>
#include<unistd.h>
#include<assert.h>

#include<cstdio>
#include<cstdlib>
#include<ctime>
#include<cstring>

#include "SortedTimerList.h"

#define MAX_EVENT_NUM 1024
#define SIG_BUF_SIZE 1024
#define SOCKET_MAX 1024
#define TIME_SLOT 5
#define MSG_BUF_SIZE 1024

static int pipefd[2];
SortedTimerList::Iterator timer_it_list[SOCKET_MAX];
int epoll_fd;
//timer_it_list[i]表示fd==i的连接的定时器

int setnonblocking(int fd){
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

int addfd(int epoll_fd,int fd){
    epoll_event event;
    event.events=EPOLLIN;
    event.data.fd=fd;
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event);
    return setnonblocking(fd);
}

void sig_handler(int sig){
    printf("get signal %d\n",sig);
    int save_errno=errno;
    write(pipefd[1],&sig,1);
    errno=save_errno;
}

void addsig(int sig){
    printf("add sig.sig=%d\n",sig);
    struct sigaction sa;
    bzero(&sa,sizeof(sa));
    sa.sa_handler=sig_handler;
    sa.sa_flags|=SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)==0);
}

void DeleteFd(int epoll_fd,int fd){
    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,fd,NULL);
    close(fd);
    printf("delete fd %d\n",fd);
}


void CloseConnect(ClientData* client_data){
    DeleteFd(epoll_fd,client_data->fd);
}

int main(int argc,char** argv){
    int i,j,ret;

    if(argc<3){
        printf("usage:%s ip port\n",argv[0]);
        return 1;
    }

    char *ip=argv[1];
    int port=atoi(argv[2]);
    printf("ip=%s port=%d\n",ip,port);

    sockaddr_in addr;
    addr.sin_family=AF_INET;
    assert(inet_pton(AF_INET,ip,&addr.sin_addr)==1);
    addr.sin_port=htons(port);
    
    int listen_fd=socket(PF_INET,SOCK_STREAM,0);
    assert(bind(listen_fd,(sockaddr*)&addr,sizeof(addr))==0);
    assert(listen(listen_fd,1024)==0);

    epoll_fd=epoll_create(1024);
    addfd(epoll_fd,listen_fd);

    assert(pipe(pipefd)==0);
    addfd(epoll_fd,pipefd[0]);

    addsig(SIGTERM);
    addsig(SIGINT);
    addsig(SIGALRM);

    //start event loop
    printf("start event loop\n");
    bool stop_server=false;
    int event_fd,conn_fd,event_num,sig_num,curr,msg_size;
    epoll_event event_list[MAX_EVENT_NUM];
    char sig_buf[SIG_BUF_SIZE+1],msg_buf[MSG_BUF_SIZE+1];
    SortedTimerList sorted_timer_list;
    alarm(TIME_SLOT);
    while(!stop_server){
        event_num=epoll_wait(epoll_fd,event_list,MAX_EVENT_NUM,-1);
        if(event_num<=0){
            printf("something error.\nevent_num=%d\nerrno=%d\n",event_num,errno);
        }else{
            for(i=0;i<event_num;i++){
                event_fd=event_list[i].data.fd;
                if(event_fd==listen_fd){
                    sockaddr_in client_addr;
                    socklen_t client_addr_len=sizeof(client_addr);
                    conn_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_addr_len);
                    if(conn_fd>0){
                        printf("new conn_fd = %d\n",conn_fd);
                        addfd(epoll_fd,conn_fd);
                        //ClientData* client_data=new ClientData{conn_fd,client_addr,NULL,""};
                        ClientData* client_data=new ClientData{conn_fd,client_addr};
                        curr=time(NULL);
                        timer_it_list[conn_fd]=sorted_timer_list.AddTimer({curr+3*TIME_SLOT,CloseConnect,client_data});
                    }
                }else if(event_fd==pipefd[0]){
                    sig_num=read(pipefd[0],sig_buf,sizeof(sig_buf));
                    for(j=0;j<sig_num;j++){
                        switch (sig_buf[j]){
                            case SIGALRM:
                                sorted_timer_list.Handle();
                                alarm(TIME_SLOT);
                                break;
                            
                            case SIGINT:
                            case SIGTERM:
                                stop_server=true;
                                break;
                        }
                    }
                }else{
                    //conn fd read
                    msg_size=recv(event_fd,msg_buf,msg_size,0);
                    if(msg_size<0){
                        printf("something wrong.\nerrno=%d\n",errno);
                    }else if(msg_size==0){
                        printf("conection close by client.\n");
                        DeleteFd(epoll_fd,event_fd);
                    }else{
                        printf("read msg:%s\n",msg_buf);
                        SortedTimerList::Iterator it=timer_it_list[event_fd];
                        Timer timer=*it;
                        int curr=time(NULL);
                        timer.expire=curr+3*TIME_SLOT;
                        timer_it_list[event_fd]=sorted_timer_list.SetTimer(it,timer);
                    }
                }
            }
        }
    }
}