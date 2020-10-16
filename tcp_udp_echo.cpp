#include<fcntl.h>
#include<sys/epoll.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>

#include<cstdio>
#include<cstdlib>
#include<cstring>

using namespace std;

#define UDP_BUFFER_SIZE 20
#define TCP_BUFFER_SIZE 10
#define MAX_EVENT_NUM 100

int setnonblocking(int fd){
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

int addfd(int epoll_fd,int fd){
    epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLIN|EPOLLET;
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
    return 0;
}

int main(int argc,char** argv){
    int ret,i;

    if(argc<3){
        printf("usage:%s ip port\n",argv[0]);
        return 1;
    }

    char *ip=argv[1];
    int port=atoi(argv[2]);

    sockaddr_in listen_addr;
    listen_addr.sin_family=AF_INET;
    ret=inet_pton(AF_INET,ip,&listen_addr.sin_addr);
    if(ret!=1){
        printf("call inet_pton() failed.errno=%d ip=%s\n",errno,ip);
        return 2;
    }
    listen_addr.sin_port=htons(port);


    int tcp_fd=socket(PF_INET,SOCK_STREAM,0);
    ret=bind(tcp_fd,(sockaddr*)&listen_addr,sizeof(listen_addr));
    if(ret!=0){
        printf("bind error.errno=%d\n",errno);
        return 3;
    }
    setnonblocking(tcp_fd);

    ret=listen(tcp_fd,1024);
    if(ret!=0){
        printf("listen error.errno=%d\n",errno);
        return 4;
    }

    int udp_fd=socket(PF_INET,SOCK_DGRAM,0);
    ret=bind(udp_fd,(sockaddr*)&listen_addr,sizeof(listen_addr));
    if(ret!=0){
        printf("bind udp error.errno=%d\n",errno);
        return 5;
    }
    setnonblocking(udp_fd);

    int epoll_fd=epoll_create(1024);
    addfd(epoll_fd,tcp_fd);
    addfd(epoll_fd,udp_fd);

    epoll_event events[MAX_EVENT_NUM];
    int event_num,event_fd;

    sockaddr_in client_addr;
    socklen_t client_addr_len;

    char udp_buf[UDP_BUFFER_SIZE+1];
    char tcp_buf[TCP_BUFFER_SIZE+1];

    //start event loop
    while(true){
        event_num=epoll_wait(epoll_fd,events,MAX_EVENT_NUM,-1);

        if(event_num<0){
            printf("call epoll_wait() failed.errno=%d\n",errno);
            return 6;
        }

        for(i=0;i<event_num;i++){
            event_fd=events[i].data.fd;
            if(event_fd==tcp_fd){
                client_addr_len=sizeof(client_addr);
                int conn_fd=accept(tcp_fd,(sockaddr*)&client_addr,&client_addr_len);

                if(conn_fd<0){
                    printf("call accept() failed.errno=%d\n",errno);
                    continue;
                }else{
                    addfd(epoll_fd,conn_fd);
                }

            }else if(event_fd==udp_fd){
                while(true){
                    bzero(udp_buf,sizeof(udp_buf));
                    client_addr_len=sizeof(client_addr);
                    ret=recvfrom(udp_fd,udp_buf,UDP_BUFFER_SIZE,0,(sockaddr*)&client_addr,&client_addr_len);
                    if(ret>=0){
                        printf("recv %d bytes of data from udp:%s\n",ret,udp_buf);
                    }else{
                        printf("errno=%d\n",errno);
                        if(errno==EAGAIN||errno==EWOULDBLOCK){
                            printf("read later\n");
                        }
                        break;
                    }
                }
            }else{
                bzero(tcp_buf,sizeof(tcp_buf));
                while(true){
                    ret=recv(event_fd,tcp_buf,TCP_BUFFER_SIZE,0);
                    if(ret>0){
                        printf("recv %d bytes of data from tcp:%s\n",ret,tcp_buf);
                    }else{
                        if(ret<0&&(errno==EAGAIN||errno==EWOULDBLOCK)){
                            printf("read later\n");
                        }else{
                            ret=epoll_ctl(epoll_fd,EPOLL_CTL_DEL,event_fd,NULL);
                            printf("call epoll_ctl to delete fd.ret=%d\n",ret);
                            close(event_fd);
                        }
                        break;
                    }
                }
            }
        }
    }

    return 0;
}