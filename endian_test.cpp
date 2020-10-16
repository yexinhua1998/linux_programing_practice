#include<cstdio>

using namespace std;

union{
    short x;
    char bytes[sizeof(short)];
}test;

int main(){
    test.x=0x1234;
    printf("%x %x\n",test.bytes[0],test.bytes[1]);
    return 0;
}