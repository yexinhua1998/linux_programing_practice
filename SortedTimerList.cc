#include "SortedTimerList.h"

#include<ctime>

using namespace std;

SortedTimerList::Iterator SortedTimerList::AddTimer(const Timer& timer){
    Iterator it;
    it=timer_list.begin();
    while(it!=timer_list.end()&&it->expire<timer.expire) it++;
    return timer_list.insert(it,timer);
}

//一般SetTimer只会往后，这里只提供往后的实现
//不考虑pos无效的情况
SortedTimerList::Iterator SortedTimerList::SetTimer(Iterator pos,const Timer& timer){
    *pos=timer;
    Iterator it=pos;
    it++;
    while(it!=timer_list.end()&&it->expire<pos->expire) it++;
    timer_list.splice(it,timer_list,pos);
}

Timer SortedTimerList::DelTimer(SortedTimerList::Iterator it){
    Timer temp=*it;
    timer_list.erase(it);
    return temp;
}

void SortedTimerList::Handle(){
    Iterator it=timer_list.begin(),old_it;
    int curr=time(NULL);
    while(it!=timer_list.end()&&it->expire<curr){
        it->callback(it->client_data);
        delete it->client_data;
        old_it=it;
        it++;
        timer_list.erase(old_it);
    }
}