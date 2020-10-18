
#include<arpa/inet.h>

#include<list>

#define CLIENT_DATA_BUFFER_SIZE 1024

struct Timer;//前向声明

struct ClientData{
    int fd;
    sockaddr_in client_addr;
    //Timer* timer;
    //char buf[CLIENT_DATA_BUFFER_SIZE];
};


struct Timer{
    int expire;//timestamp in second
    void (*callback)(ClientData*);
    ClientData* client_data;
};

class SortedTimerList{
public:
    using Iterator=std::list<Timer>::iterator;
    Iterator AddTimer(const Timer& timer);
    Iterator SetTimer(Iterator pos,const Timer& timer);
    Timer DelTimer(Iterator it);
    void Handle();
private:
    std::list<Timer> timer_list;
};