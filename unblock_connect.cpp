#include<fcntl.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/select.h>
#include<errno.h>

#include<cstdio>
#include<cstdlib>
#include<vector>

using namespace std;

int setnonblocking(int fd){
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

//同时发起n个连接
int concurrent_connect(int concurrent_num,char* ip,int port){
    int sock_list[concurrent_num];
    int i,ret;

    sockaddr_in addr;
    addr.sin_family=AF_INET;
    ret=inet_pton(AF_INET,ip,&addr.sin_addr);
    if(ret!=1){
        printf("call inet_pton() failed.ret=%d ip=%s\n",ret,ip);
        return -1;
    }
    addr.sin_port=htons(port);

    fd_set writefds;
    FD_ZERO(&writefds);

    int conn_fd;
    vector<int> conn_fd_list;
    for(i=0;i<concurrent_num;i++){
        printf("a loop.i=%d\n",i);

        conn_fd=socket(PF_INET,SOCK_STREAM,0);
        setnonblocking(conn_fd);
        printf("call connect().\n");
        ret=connect(conn_fd,(sockaddr*)&addr,sizeof(addr));

        if(ret==0){
            //连接立刻完成
            printf("connect complete.");
        }else if(errno!=EINPROGRESS){
            //出现了其他错误
            printf("find other error.errno=%d\n",errno);
            continue;
        }



        FD_SET(conn_fd,&writefds);
        conn_fd_list.push_back(conn_fd);
    }

    //调用select
    printf("call select.");
    ret=select(conn_fd+1,NULL,&writefds,NULL,NULL);
    printf("select return %d\n",ret);

    char buf[128];
    for(int conn_fd:conn_fd_list){
        if(FD_ISSET(conn_fd,&writefds)){
            printf("socket %d is ready to write.\n",conn_fd);
            sprintf(buf,"hi,i am socket %d\n",conn_fd);
            ret=send(conn_fd,buf,sizeof(buf),0);
            printf("ret=%d\n",ret);
        }
    }

    return 0;
}

int main(int argc,char** argv){
    int ret;

    if(argc<3){
        printf("usage:%s ip port\n",argv[0]);
        return 1;
    }

    concurrent_connect(10,argv[1],atoi(argv[2]));
    return 0;
}