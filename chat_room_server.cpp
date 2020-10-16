#include<poll.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>
#include<fcntl.h>

#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<queue>
#include<string>
#include<algorithm>

using namespace std;

#define FD_MAX 65535
#define BUFFER_SIZE 1024

struct ClientData{
    sockaddr_in addr;
    queue<string> mq;//message queue
};

ClientData client_list[FD_MAX];//用fd作为索引获取用户信息，即fd为user_id

int setnonblocking(int fd){
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

int main(int argc,char** argv){
    int i,j,ret;

    if(argc<3){
        printf("usage:%s ip port\n",argv[0]);
        return 1;
    }

    char *ip=argv[1];
    int port=atoi(argv[2]);

    sockaddr_in addr;
    addr.sin_family=AF_INET;
    ret=inet_pton(AF_INET,ip,&addr.sin_addr);
    if(ret!=1){
        printf("call inet_pton() failed.ret=%d ip=%s errno=%d\n",ret,ip,errno);
        return 2;
    }
    addr.sin_port=htons(port);

    int listen_fd=socket(PF_INET,SOCK_STREAM,0);
    ret=bind(listen_fd,(sockaddr*)&addr,sizeof(addr));
    if(ret!=0){
        printf("call bind() failed.ret=%d errno=%d\n",ret,errno);
        return 3;
    }

    ret=listen(listen_fd,1024);
    if(ret!=0){
        printf("call listen failed.ret=%d errno=%d\n",ret,errno);
        return 4;
    }

    pollfd poll_fds[FD_MAX];
    int user_num=0,recver_fd;
    poll_fds[0].fd=listen_fd;
    poll_fds[0].events=POLLIN;
    char buf[BUFFER_SIZE+1];//最后一个字节做\0
    string message;

    //start event loop
    while(true){

        ret=poll(poll_fds,user_num+1,-1);

        //有新的连接
        if(poll_fds[0].revents&POLLIN){
            sockaddr_in client_addr;
            socklen_t client_addr_len=sizeof(client_addr);
            int conn_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_addr_len);
            if(conn_fd<0){
                printf("call accept failed.errno=%d\n",errno);
            }else{
                user_num++;
                poll_fds[user_num].fd=conn_fd;
                poll_fds[user_num].events=POLLIN;
                client_list[conn_fd].addr=client_addr;
                setnonblocking(conn_fd);
            }
        }

        for(i=1;i<=user_num;i++){
            //socket可读
            if(poll_fds[i].revents&POLLIN){
                bzero(buf,sizeof(buf));
                ret=recv(poll_fds[i].fd,buf,BUFFER_SIZE,0);
                if(ret==0){
                    //客户端关闭连接，删除该连接
                    close(poll_fds[i].fd);
                    swap(poll_fds[i],poll_fds[user_num]);
                    user_num--;
                    i--;
                }else if(ret<0){
                    printf("call recv() error.errno=%d\n",errno);
                }else{
                    printf("recv buf:\n%s\n",buf);
                    message=buf;
                    for(j=1;j<=user_num;j++){
                        if(i==j) continue;
                        recver_fd=poll_fds[j].fd;
                        client_list[recver_fd].mq.push(message);
                        poll_fds[i].revents|=POLLOUT;
                    }
                }
            }

            //可写
            //为了提高吞吐，采用nonblocking的方式去写
            if(poll_fds[i].revents&POLLOUT){
                recver_fd=poll_fds[i].fd;
                while(true){
                    if(client_list[recver_fd].mq.empty()){
                        poll_fds[i].events&=~POLLOUT;//取消POLLOUT印记
                        break;
                    }else{
                        message=client_list[recver_fd].mq.front();
                        bzero(buf,sizeof(buf));
                        memcpy(buf,message.c_str(),message.size());
                        printf("ready to push message in buf:%s\n",buf);
                        ret=send(recver_fd,buf,message.size()+1,0);
                        if(ret<0){
                            if(errno==EAGAIN||errno==EWOULDBLOCK){
                                printf("write later\n");
                            }else{
                                printf("failed to send data to client.errno=%d\n",errno);
                            }
                            break;
                        }else{
                            printf("success to send %d bytes to client.\n",ret);
                            client_list[recver_fd].mq.pop();
                        }
                    }
                }
                
            }
        }
    }

    return 0;
}