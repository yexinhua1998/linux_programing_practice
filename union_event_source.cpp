#include<fcntl.h>
#include<sys/epoll.h>
#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<assert.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<string.h>

#define MAX_EVENT_NUM 1024
#define SIG_BUF_SIZE 1024
#define MSG_BUF_SIZE 1024

static int pipefd[2];

int setnonblocking(int fd){
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

void addfd(int epoll_fd,int fd){
    epoll_event event;
    event.events=EPOLLIN|EPOLLET;
    event.data.fd=fd;
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

void sighandle(int sig){
    int save_errno=errno;
    int msg=sig;
    printf("sighandle:get signal %d\n",sig);
    int ret=send(pipefd[1],(char*)&msg,1,0);
    printf("send:return=%d\n",ret);
    if(ret==-1) printf("send error.errno=%d\n",errno);
    errno=save_errno;
}

void addsig(int sig){
    struct sigaction sa;
    sa.sa_handler=sighandle;
    sa.sa_flags|=SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)==0);
}

int main(int argc,char** argv){
    int ret,i,j;

    if(argc<3){
        printf("usage:%s ip port\n",argv[0]);
        return 1;
    }

    char *ip=argv[1];
    int port=atoi(argv[2]);

    printf("ip=%s port=%d\n",ip,port);

    int listen_fd=socket(PF_INET,SOCK_STREAM,0);
    assert(listen>0);

    sockaddr_in listen_addr;
    listen_addr.sin_family=AF_INET;
    ret=inet_pton(AF_INET,ip,&listen_addr.sin_addr);
    assert(ret==1);
    listen_addr.sin_port=htons(port);

    ret=bind(listen_fd,(sockaddr*)&listen_addr,sizeof(listen_addr));
    assert(ret==0);

    ret=listen(listen_fd,1024);
    assert(ret==0);
    
    int epollfd=epoll_create(1024);
    addfd(epollfd,listen_fd);

    ret=socketpair(PF_UNIX,SOCK_STREAM,0,pipefd);
    assert(ret==0);
    addfd(epollfd,pipefd[0]);

    addsig(SIGHUP);
    addsig(SIGCHLD);
    addsig(SIGTERM);
    addsig(SIGINT);

    //start event loop
    bool run=true;
    epoll_event event_list[MAX_EVENT_NUM];
    char sigbuf[SIG_BUF_SIZE],msgbuf[MSG_BUF_SIZE+1];
    int signum,sigvalue,eventfd;
    printf("start event loop\n");
    while(run){
        ret=epoll_wait(epollfd,event_list,MAX_EVENT_NUM,-1);
        if(ret==-1&&errno==EINTR){
            continue;
        }
        for(i=0;i<ret;i++){
            eventfd=event_list[i].data.fd;
            if(eventfd==listen_fd){
                sockaddr_in client_addr;
                socklen_t client_addr_len;
                int conn_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_addr_len);
                if(conn_fd>=0){
                    addfd(epollfd,conn_fd);
                    printf("get new fd %d\n",conn_fd);
                }
            }else if(eventfd==pipefd[0]){
                bzero(sigbuf,sizeof(sigbuf));
                signum=recv(eventfd,sigbuf,SIG_BUF_SIZE,0);
                if(signum<0) continue;
                for(j=0;j<signum;j++){
                    sigvalue=sigbuf[j];
                    printf("recv sig %d\n",sigvalue);
                    switch (sigvalue){
                        case SIGCHLD:
                        case SIGHUP:
                            continue;
                        case SIGTERM:
                        case SIGINT:
                            run=false;
                    }
                }
            }else{
                bzero(msgbuf,sizeof(msgbuf));
                ret=recv(eventfd,msgbuf,MSG_BUF_SIZE,0);
                if(ret>0){
                    printf("recv msg %s\n",msgbuf);
                }
            }
        }
    }

    close(listen_fd);
    close(epollfd);
    return 0;
}