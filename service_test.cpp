#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<cstdio>
using namespace std;

int main(int argc,char** argv){
    if(argc<2){
        printf("usage: %s host\n",argv[0]);
        return -1;
    }
    hostent* hostinfo=gethostbyname(argv[1]);
    servent* service_info=getservbyname("daytime","tcp");
    printf("host name:%s\nservice_name:%s\nservice_port:%d\n",hostinfo->h_name,service_info->s_name,ntohs(service_info->s_port));

    //specify the address to connect

    sockaddr_in socket_addr;
    socket_addr.sin_family=AF_INET;
    socket_addr.sin_port=service_info->s_port;
    socket_addr.sin_addr=*(in_addr*)(*(hostinfo->h_addr_list));

    //create socket and connect to it 

    int sock_fd=socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd<0){
        printf("create socket failed\n");
        return 1;
    }
    int result=connect(sock_fd,(sockaddr*)&socket_addr,sizeof(socket_addr));
    printf("connect:result=%d\n",result);

    char buf[1024];
    result=read(sock_fd,buf,sizeof(buf));
    printf("read.result:%d\n",result);

    if(result!=-1){
        buf[result]='\0';
        printf("buf=%s\n",buf);
    }
    return 0;
}