#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

int main(){
    in_addr addr;
    addr.s_addr=0x01020304;
    char *szValue1=inet_ntoa(addr);
    addr.s_addr=0x05060708;
    char *szValue2=inet_ntoa(addr);
    printf("v1=%s v2=%s\n",szValue1,szValue2);
    printf("v1=%x v2=%x\n",szValue1,szValue2);
    addr.s_addr=inet_addr("3.4.5.6");
    printf("addr in binary = %x\n",addr.s_addr);

    int ret;
    ret=inet_aton("2.3.4.5",&addr);
    printf("last addr:%x\n",addr.s_addr);
    return 0;
}
