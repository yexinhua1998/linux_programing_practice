#include<sys/select.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>

#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<vector>
#include<set>

using namespace std;

int main(int argc,char** argv){
    int ret;

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
        printf("call inet_pton() failed.ret=%d ip=%s\n",ret,ip);
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
        printf("call listen() error.ret=%d errno=%d\n",ret,errno);
        return 4;
    }

    fd_set read_fds;
    FD_SET(listen_fd,&read_fds);
    int max_fd=listen_fd;
    set<int> conn_fd_set;
    vector<int> invalid_conn_fd_list;
    char buf[1024];
    sockaddr_in client_addr;
    socklen_t client_addr_len;
    //event loop start
    while(true){
        printf("call a loop\n");

        ret=select(max_fd+1,&read_fds,NULL,NULL,NULL);

        for(const int& conn_fd:conn_fd_set){
            if(FD_ISSET(conn_fd,&read_fds)){
                bzero(buf,sizeof(buf));
                ret=recv(conn_fd,buf,sizeof(buf),0);
                printf("get buf:\n%s\n",buf);
                if(ret==0){
                    invalid_conn_fd_list.push_back(conn_fd);
                    close(conn_fd);
                }
            }
        }

        for(const int& conn_fd:invalid_conn_fd_list){
            conn_fd_set.erase(conn_fd);
        }

        if(FD_ISSET(listen_fd,&read_fds)){
            printf("try to accept.\n");
            client_addr_len=sizeof(client_addr);
            int new_conn_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_addr_len);
            printf("new conn fd=%d\n",new_conn_fd);
            if(new_conn_fd<0){
                printf("call accept error.errno=%d\n",errno);
            }else{
                max_fd=new_conn_fd>max_fd?new_conn_fd:max_fd;
                conn_fd_set.insert(new_conn_fd);
            }
        }

        FD_ZERO(&read_fds);
        FD_SET(listen_fd,&read_fds);
        for(const int& conn_fd:conn_fd_set){
            FD_SET(conn_fd,&read_fds);
        }
    }
    return 0;
}