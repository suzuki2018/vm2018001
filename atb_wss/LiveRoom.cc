#include "LiveRoom.h"
#include <algorithm>

LiveRoom::LiveRoom(std::string cmsid, std::string devid, std::string token):cmsid_(cmsid),devid_(devid),token_(token){
                timestamp_ = 0;
		rdy_ = false;
}

LiveRoom::~LiveRoom(){

}

int LiveRoom::size()
{
    int size = 0;
    slocker_.lock();
    size = users_.size();
    slocker_.unlock();
    return size;
}

int LiveRoom::join(int fd){

    int size = 0;
    slocker_.lock();
    std::vector<int>::iterator it = std::find( users_.begin(), users_.end(), fd);
    if(it!=users_.end())
    {
	size = users_.size();
	slocker_.unlock();
	return size;
    }
    users_.push_back(fd);
    size = users_.size();
    if(size>0)
    {
        timestamp_ = 0;
    }
    slocker_.unlock();

    LOG_INFO("[wss]: camera play[%s] --> id(%d) join --> size(%d)",devid_.c_str(),fd,size);
    return size;
}

int  LiveRoom::quit(int fd){
     int size = 0;
     slocker_.lock();
     std::vector<int>::iterator it = std::find( users_.begin( ), users_.end( ), fd);
     if(it==users_.end())
     {
         size = users_.size();
         slocker_.unlock();
         return size;
     }
	
     users_.erase(it);
     size = users_.size();

     if(size<1)
     {
         timestamp_ = Global::GetTickCount();//start dead time count
     }
     slocker_.unlock();
     LOG_INFO("[wss]: camera play[%s] --> id(%d) quit --> size(%d)",devid_.c_str(),fd,size);
     return size;
}

void LiveRoom::show_users()
{
#if 0
    std::string strusers = "";
    char szTmp[12]={0};
    int  count = 0;

    slocker_.lock();
    std::vector<int>::iterator iter;  
    for (iter=users_.begin();iter!=users_.end();iter++)  
    {  
	sprintf(szTmp,"%05d ",*iter);
	strusers+=szTmp;
	if(++count%20==0) strusers+="\r\n";
    }  
    slocker_.unlock();

    LOG_INFO("[wss]: camera play(%s): %s",devid_.c_str(),strusers.c_str());
#endif
    int count = 0;
    slocker_.lock();
    count = users_.size();
    slocker_.unlock();
    LOG_INFO("[wss]:camera -> %s(%d)",devid_.c_str(),count);


}


