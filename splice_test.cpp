#include<fcntl.h>//spice
#include<netinet/in.h>//struct sockaddr_in
#include<arpa/inet.h>//inet_pton
#include<errno.h>
#include<unistd.h>

#include<cstdio>
#include<cstdlib>
#include<cstring>

using namespace std;

int main(int argc,char** argv){
    int ret;

    if(argc<3){
        printf("usage:%s ip port\n",argv[0]);
        return 1;
    }

    char* ip=argv[1];
    int port=atoi(argv[2]);

    sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family=AF_INET;
    ret=inet_pton(AF_INET,ip,&addr.sin_addr);
    if(ret!=1){
        printf("inet_pton return error.ret=%d,errorno=%d\n",ret,errno);
        return 2;
    }
    addr.sin_port=htons(port);

    int listen_fd=socket(PF_INET,SOCK_STREAM,0);
    if(listen_fd==-1){
        printf("create socket error.error no=%d\n",errno);
        return 3;
    }

    ret=bind(listen_fd,(sockaddr*)&addr,sizeof(addr));
    if(ret==-1){
        printf("bind error.error no = %d\n",errno);
        return 4;
    }

    ret=listen(listen_fd,5);
    if(ret==-1){
        printf("listen fail.\nerrno=%d\n",errno);
        return 5;
    }

    sockaddr_in client_addr;
    socklen_t client_addr_len;
    int conn_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_addr_len);
    if(conn_fd==-1){
        printf("accept error.errno=%d\n",errno);
        return 6;
    }

    //create pipe
    int pipe_fd[2];
    ret=pipe(pipe_fd);
    if(ret!=0){
        printf("pipe create error.errno=%d\n",errno);
        return 7;
    }

    while(true){
        printf("a loop\n");
        ret=splice(conn_fd,NULL,pipe_fd[1],NULL,1024,SPLICE_F_MOVE|SPLICE_F_MORE);
        if(ret==-1){
            printf("splice 1 error.errno=%d\n",errno);
            return 8;
        }
        ret=splice(pipe_fd[0],NULL,conn_fd,NULL,1024,SPLICE_F_MOVE|SPLICE_F_MORE);
        if(ret==-1){
            printf("splice 2 error.errno=%d\n",errno);
            return 9;
        }
    }

    close(conn_fd);
    close(listen_fd);
    return 0;
}
