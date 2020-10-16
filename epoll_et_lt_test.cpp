#include<sys/epoll.h>//epoll_wait,epoll_event
#include<netinet/in.h>//struct sockaddr_in
#include<arpa/inet.h>//inet_pton
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define MAX_EVENT_NUM 1024
#define BUFFER_SIZE 10

//设置socket为非阻塞
int setnonblocking(int fd){
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option|O_NONBLOCK;
    int ret=fcntl(fd,F_SETFL,new_option);
    if(ret!=0){
        printf("call fcntl failed.ret=%d,errno=%d\n",ret,errno);
        return -1;
    }
    return new_option;
}

//向事件表增加fd
void addfd(int epoll_fd,int fd,bool enable_et){
    epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLIN;
    if(enable_et){
        event.events|=EPOLLET;
    }
    int ret=epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event);
    if(ret!=0){
        printf("add fd into events table failed.ret=%d,errno=%d\n",ret,errno);
        return ;
    }
    setnonblocking(fd);
}

//level triger方式
int lt(epoll_event* events,int event_num,int epoll_fd,int listen_fd){
    char buf[BUFFER_SIZE];
    int i,ret;

    printf("call lt().event_num=%d\n",event_num);
    for(i=0;i<event_num;i++){
        int socket_fd=events[i].data.fd;
        if(socket_fd==listen_fd){
            struct sockaddr_in client_addr;
            socklen_t client_addr_len;
            int conn_fd=accept(socket_fd,(sockaddr*)&client_addr,&client_addr_len);
            if(conn_fd==-1){
                printf("call accept error.errno=%d\n",errno);
                continue;
            }
            addfd(epoll_fd,conn_fd,false);
        }else if(events[i].events&EPOLLIN){
            //监听的socket readable
            bzero(buf,sizeof(buf));
            ret=recv(socket_fd,buf,BUFFER_SIZE-1,0);//-1是因为最后一个字符要当'\0'
            if(ret==-1){
                printf("call recv error.errno=%d\n",errno);
            }else{
                printf("get %d bytes of data.content:%s\n",ret,buf);
            }
        }else{
            printf("something other happen.\n");
        }
    }
}

int et(epoll_event* events,int event_num,int epoll_fd,int listen_fd){
    char buf[BUFFER_SIZE];
    int i,ret;
    printf("call et\n");

    for(i=0;i<event_num;i++){
        int event_fd=events[i].data.fd;
        if(event_fd==listen_fd){
            sockaddr_in client_addr;
            socklen_t client_addr_len;
            int conn_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_addr_len);
            if(conn_fd==-1){
                printf("accept error.errno=%d\n",errno);
            }else{
                addfd(epoll_fd,conn_fd,true);
            }
        }else if(events[i].events&EPOLLIN){
            while(true){
                bzero(buf,BUFFER_SIZE);
                printf("call recv.\n");
                ret=recv(event_fd,buf,BUFFER_SIZE-1,0);
                if(ret>0){
                    printf("get %d bytes of data.content=%s\n",ret,buf);
                }else if(ret==0){
                    close(epoll_fd);
                    break;
                }else{
                    if(errno==EAGAIN||errno==EWOULDBLOCK){
                        //没有数据可以读了
                        printf("read later\n");
                    }else{
                        //事实上应该还要加上将其从events表中移除
                        close(event_fd);
                    }
                    break;
                }
            }
        }else{
            printf("something else happen.\n");
        }
    }
}

int main(int argc,char** argv){
    int ret;

    if(argc<3){
        printf("usage:%s ip port\n",argv[0]);
        return 1;
    }

    char *ip=argv[1];
    int port=atoi(argv[2]);

    struct sockaddr_in listen_addr;
    listen_addr.sin_family=AF_INET;
    listen_addr.sin_port=htons(port);
    ret=inet_pton(AF_INET,ip,&listen_addr.sin_addr);
    if(ret!=1){
        printf("call inet_pton fail.ret=%d,ip=%s\n",ret,ip);
        return 2;
    }

    int listen_fd=socket(PF_INET,SOCK_STREAM,0);
    if(listen_fd==-1){
        printf("call socket() error.errno=%d\n",errno);
        return 3;
    }

    ret=bind(listen_fd,(sockaddr*)&listen_addr,sizeof(listen_addr));
    if(ret!=0){
        printf("call bind() error.errno=%d\n",errno);
        return 4;
    }

    ret=listen(listen_fd,1024);
    if(ret!=0){
        printf("call listen failed.errno=%d\n",errno);
        return 5;
    }

    int epoll_fd=epoll_create(1024);
    if(epoll_fd==-1){
        printf("call epoll_create() fail.errno=%d\n",errno);
        return 6;
    }
    addfd(epoll_fd,listen_fd,true);

    epoll_event events[MAX_EVENT_NUM];
    while(true){
        printf("call epoll event.\n");
        ret=epoll_wait(epoll_fd,events,MAX_EVENT_NUM,-1);
        printf("block end.\n");
        if(ret==-1){
            printf("call epoll_wait fail.errno=%d\n",errno);
            return 7;
        }
        et(events,ret,epoll_fd,listen_fd);
    }

    return 0;
}