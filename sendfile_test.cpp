#include<stdio.h>//printf
#include<stdlib.h>//atoi
#include<unistd.h>//close
#include<fcntl.h>//open
#include<sys/stat.h>//fstat,struct stat
#include<sys/socket.h>//struct sockaddr,bind,listen,accept
#include<netinet/in.h>//struct sockaddr_in
#include<arpa/inet.h>//inet_pton
#include<sys/sendfile.h>//sendfile


int main(int argc,char** argv){
    int ret;
    if(argc<4){
        printf("usage: %s ip port filename",argv[0]);
        return 1;
    }
    char* ip=argv[1];
    int port=atoi(argv[2]);
    char* filename=argv[3];

    int file_fd=open(filename,O_RDONLY);
    if(file_fd==-1){
        printf("open file error.filename=%s\n",filename);
        return 2;
    }
    struct stat statbuf;//这里需要用struct来表明stat是结构体，否则编译错误
    ret=fstat(file_fd,&statbuf);
    if(ret==-1){
        printf("fstat error\n");
        return 3;
    }

    sockaddr_in addr;
    addr.sin_family=AF_INET;
    ret=inet_pton(AF_INET,ip,&addr.sin_addr);
    if(ret==-1){
        printf("inet_pton error.ret=%d ip=%s addr=%x\n",ret,ip,addr.sin_addr.s_addr);
        return 4;
    }
    addr.sin_port=htons(port);

    int listen_fd=socket(PF_INET,SOCK_STREAM,0);
    if(listen_fd==-1){
        printf("create socket error.listen_fd=%d\n",listen_fd);
        return 5;
    }

    ret=bind(listen_fd,(sockaddr*)&addr,sizeof(addr));
    if(ret==-1){
        printf("bind error.");
        return 6;
    }

    ret=listen(listen_fd,5);
    
    sockaddr_in client_addr;
    socklen_t client_addr_len;
    int conn_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_addr_len);
    if(conn_fd==-1){
        printf("accept error.\n");
        return 7;
    }

    ret=sendfile(conn_fd,file_fd,NULL,statbuf.st_size);

    close(conn_fd);
    close(listen_fd);

    return 0;
}