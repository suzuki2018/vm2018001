#include "PlayMngr.h"
#include <unistd.h>
#include <pthread.h>
#include "LiveRoom.h"

#include "atb_sak.h"

void handle_func(void *data) {
    PMTaskData *td = (PMTaskData *) data;
    td->pm->handle_request(td->socketid,td->cmdline);
    delete td;
}

PlayMngr::PlayMngr(int port): WebSocketServer( port )
{
	state_pid_ = 0;
	pid_ = 0;
	bRunning_ = false;
	handler_ = NULL;
	pthread_mutex_init(&roomlock_, NULL);
}

PlayMngr::~PlayMngr()
{

}


void PlayMngr::onConnect( int socketID )
{
    //LOG_INFO("[wss]:id(%d) ----> in",socketID);
    // Give this connection a random user ID
    const string& handle = "User #" + Util::toString( socketID );
    Util::log( "New connection: " + handle );

    // Associate this handle with the connection
    this->setValue( socketID, "handle", handle );
    unsigned long cur = Global::GetTickCount();
    this->updateTimestamp(socketID,cur);
    // Let everyone know the new user has connected
//    this->broadcast( handle + " has connected." );
}

void PlayMngr::onMessage( int socketID, const string& data )
{
     PMTaskData *tdata = new PMTaskData();
     tdata->cmdline = data;
     tdata->socketid = socketID;
     tdata->pm = this;

     Task *task = new Task(handle_func, tdata);
     int ret = threadpool_->add_task(task);
     if (ret != 0) {
            LOG_INFO("create task fail:%d, we will close connect.", ret);
            
	    this->_removeConnection( socketID );
	    delete tdata;
            delete task;
        }

    //LOG_INFO("[wss]: user message(%d) -->message:%s",socketID,data.c_str());

#if 0
    // Send the received message to all connected clients in the form of 'User XX: message...'
    LOG_INFO("[wss]: user message(%d) -->message:%s\r\n",socketID,data.c_str());
    Util::log( "Received: " + data );
    const string& message = this->getValue( socketID, "handle" ) + ": " + data;

    this->broadcast( message );
#endif
}


void PlayMngr::onTimeout(int socketid, std::string roomid)
{
     if(roomid.size()==0) return;
     //LOG_INFO("[wss]: id(%d) timeout",socketid);
     LiveRoom *pRoom = NULL;
     pthread_mutex_lock(&roomlock_);
     std::map<string,LiveRoom*>::iterator iter = rooms_.find(roomid);
     if( rooms_.end() != iter )
     {
           pRoom = iter->second;
     }
     if(pRoom!=NULL) pRoom->quit(socketid);
     pthread_mutex_unlock(&roomlock_);

}

void PlayMngr::onDisconnect( int socketID )
{
    //LOG_INFO("[wss]: id(%d) ----> out",socketID);
    string roomid = this->getValue(socketID, "roomid");
    
    if(roomid.size()==0) return;

     LiveRoom *pRoom = NULL;
     pthread_mutex_lock(&roomlock_);
     std::map<string,LiveRoom*>::iterator iter = rooms_.find(roomid);
     if( rooms_.end() != iter )
     {
           pRoom = iter->second;
     }
     if(pRoom!=NULL) pRoom->quit(socketID);     
     pthread_mutex_unlock(&roomlock_);



#if 0
    const string& handle = this->getValue( socketID, "handle" );
    Util::log( "Disconnected: " + handle );

    // Let everyone know the user has disconnected
    const string& message = handle + " has disconnected.";
    for( map<int,Connection*>::const_iterator it = this->connections.begin( ); it != this->connections.end( ); ++it )
        if( it->first != socketID )
            // The disconnected connection gets deleted after this function runs, so don't try to send to it
            // (It's still around in case the implementing class wants to perform any clean up actions)
            this->send( it->first, message );
#endif
}

void PlayMngr::onError( int socketID, const string& message )
{
    Util::log( "Error: " + message );
    LOG_INFO("[wss]: user err -->id(%d)",socketID);
}



int  PlayMngr::Start(PlayctrlHandler *handler,ThreadPool *threadpool)
{
	handler_ = handler;
	threadpool_ = threadpool;
  
	if(start_async()!=0)
	{
		Stop();
    		LOG_INFO("[WSS]: Websocket context create fail.");
		return -1;
	}
	
    	LOG_INFO("[WSS]: Websocket context create success.");
	return 0;
}


void PlayMngr::Stop()
{
}


void *playmngr_start_routine(void *ptr) {
    PlayMngr *hs = (PlayMngr *) ptr;
    hs->start_sync();
    return NULL;
}

void *PlayMngr::state_thread(void *ptr){
    PlayMngr *pthis = (PlayMngr *)ptr;
    pthis->state_threadfunc();

   return NULL;
	
}

int  PlayMngr::state_threadfunc()
{
	int iCount0 = 0;	
	int iCount1 = 0;
	int i = 0;
	while(1)
	{
		
		for(i = 0; i< 100; i++)
		{
			if(!bRunning_) break;
			usleep(10*1000);
		}

#if 1
		if(++iCount0>12)//heartbeat 15seconds
		{
			//LOG_INFO("check connection state....\r\n");
			unsigned long cur0 = Global::GetTickCount();
                	this->CheckConnectionState(cur0);
			iCount0 = 0;
		}
#endif
		if(++iCount1<9)
		{
			continue;
		}	
		else
		{
			iCount1=0;
		}
		//LOG_INFO("check liveroom state...\r\n");
		pthread_mutex_lock(&roomlock_);
		for( map<string,LiveRoom*>::const_iterator it = this->rooms_.begin( ); it != this->rooms_.end( );)
		{
			LiveRoom *p = it->second;
			std::string devid = p->devid_;
			unsigned long cur = Global::GetTickCount();
			if(p->size()==0 && (cur-p->timestamp_)>60*1000)
			{
				string result;
				handler_->stop_play(p->cmsid_,p->devid_,result);
				delete p;
				p = NULL;
				it=rooms_.erase(it);
				LOG_INFO("[wss]: camera play(%s) ---> stop",devid.c_str());
			}
			else
			{
				p->show_users();
				it++;
			}
		}
		pthread_mutex_unlock(&roomlock_);
	}
	return 0;
}

int PlayMngr::NotifyHls(std::string token)
{
	LiveRoom *p = NULL;
        pthread_mutex_lock(&roomlock_);
        for( map<string,LiveRoom*>::const_iterator it = this->rooms_.begin( ); it != this->rooms_.end( );it++)	
	{
		LiveRoom *tmp = it->second;
		if(tmp->token_==token)
		{
			//LOG_INFO("[wss]: find!!!");
			p = tmp;
			break;
		}
	}
        pthread_mutex_unlock(&roomlock_);

	if(p==NULL) return -1;
	p->rdy_ = true;
	int isize = p->size();
	p->slocker_.lock();
        Json::Value resp;
        resp["code"]=101;
        resp["result"]="success";
        resp["token"]=p->token_;
        resp["num"]=isize;
	//LOG_INFO("[wss]: room size(%s) --> %d",token.c_str(),isize);
        string strresp = resp.toStyledString();
    	std::vector<int>::iterator iter;
    	for (iter=p->users_.begin();iter!=p->users_.end();iter++)
    	{
		this->send(*iter,strresp);
    	}
    	p->slocker_.unlock();	

	return 0;
}

int PlayMngr::start_async() {

    bRunning_ = true;
    int ret = pthread_create(&pid_, NULL, playmngr_start_routine, this);
    if (ret != 0) {
        LOG_INFO("HttpServer::start_async err:%d", ret);
    	bRunning_ = false;
        return ret;
    }

    ret = pthread_create(&state_pid_, NULL, state_thread, this);


    return 0;
}


int  PlayMngr::start_sync()
{
	this->run();
	return 0;
}


int  PlayMngr::handle_request(int socketid, string cmdline)
{
	Json::Reader reader;
 	Json::Value value;
	if (!reader.parse(cmdline, value))
	{
		this->_removeConnection( socketid );
		return -1;
	}
	
	int cmd = -1;
	string cmsid,devid;
	try{
		cmd = value["cmd"].asInt();
		cmsid = value["cmsid"].asString();
		devid = value["devid"].asString();

	}catch(...)
	{
		this->_removeConnection( socketid );
                return -1;
	}

	
	if(cmd==100)
	{
		return join_liveroom(socketid,cmsid,devid);
	}
	else if(cmd==200)
	{
		return quit_liveroom(socketid,cmsid,devid);
	}
	else if(cmd==300)
	{
		unsigned long cur = Global::GetTickCount();
                this->updateTimestamp(socketid,cur);
		std::string roomid = this->getValue(socketid,"roomid");
		int isize = roomid.size();
		Json::Value resp0;
                resp0["code"]=(isize==0)?302:301;
                std::string strresp0 = resp0.toStyledString();
                this->send(socketid, strresp0);
	}
	else{
		this->_removeConnection( socketid );
                return -1;
	}

	return 0;
}

int  PlayMngr::join_liveroom(int socketid, string cmsid, string devid)
{

	int ret = handler_->check_play_status(cmsid,devid);
	//LOG_INFO("[wss]:join room -> socketid(%d) -> camera(%s) status(%d)", socketid,devid.c_str(),ret);

	//ret=100:adapter client offline
	//ret=101:device offline 
	//ret=102:device not playing
	//ret=200:device playing

	LiveRoom *pRoom = NULL;
	pthread_mutex_lock(&roomlock_);
        std::map<string,LiveRoom*>::iterator iter = rooms_.find(cmsid+devid);
        if( rooms_.end() != iter )
	{
		pRoom = iter->second;
	}

	if(ret==100||ret==101)
	{
		if(pRoom!=NULL)
		{
			delete pRoom;
			pRoom = NULL;
			rooms_.erase(iter);
		}
		pthread_mutex_unlock(&roomlock_);

		Json::Value resp;
                resp["code"]=103;
                resp["result"]="camera offline!";
		string strresp = resp.toStyledString();
		LOG_INFO("[wss]: id(%d) join @ cmsid(%s) devid(%s) ---> camera offline",socketid,cmsid.c_str(),devid.c_str());
                this->send(socketid, strresp);
                return -1;
	}
	else if(ret==102)
	{
		if(pRoom!=NULL)
		{
			delete pRoom;
			pRoom = NULL;
			rooms_.erase(iter);
		}
		pthread_mutex_unlock(&roomlock_);
		
		string result,token;
		int res = handler_->start_play(cmsid,devid,result,token);	
		if(res!=0)
		{
			Json::Value resp;
			resp["code"]=104;
			resp["result"]=result;
			string strresp = resp.toStyledString();
                	this->send(socketid, strresp);
			LOG_INFO("[wss]: id(%d) join @ cmsid(%s) devid(%s) ---> %s",socketid,cmsid.c_str(),devid.c_str(),result.c_str());
			return -1;
		}
		else
		{
			LiveRoom *p = new LiveRoom(cmsid,devid, token);
			int count = p->join(socketid);
			pthread_mutex_lock(&roomlock_);
			rooms_[cmsid+devid] = p;
			pthread_mutex_unlock(&roomlock_);
			Json::Value resp;
			resp["code"]=102;
			resp["result"]="stream not ready!";
			resp["token"]=token;
			resp["num"]=count;
			string strresp = resp.toStyledString();
               		this->send(socketid, strresp);
                        this->setValue(socketid,"roomid",cmsid+devid);
			unsigned long cur = Global::GetTickCount();
			this->updateTimestamp(socketid,cur);
			LOG_INFO("[wss]: id(%d) join @ cmsid(%s) devid(%s) ---> success(%s)",socketid,cmsid.c_str(),devid.c_str(),token.c_str());
			return 0;
		}

	}
	else if(ret==200)
	{
		pthread_mutex_unlock(&roomlock_);
		if(pRoom!=NULL)
		{
			int count = pRoom->join(socketid);
			bool bRdy = pRoom->rdy_;
			if(!bRdy){
				std::string token = pRoom->get_token();
                        	Json::Value resp;
                        	resp["code"]=102;
                        	resp["result"]="stream not ready!";
                        	resp["token"]=token;
                        	resp["num"]=count;
                        	string strresp = resp.toStyledString();
                        	this->send(socketid, strresp);
                        	this->setValue(socketid,"roomid",cmsid+devid);
                        	unsigned long cur = Global::GetTickCount();
                        	this->updateTimestamp(socketid,cur);
                        	LOG_INFO("[wss]: id(%d) join @ cmsid(%s) devid(%s)--> success(%s)",socketid,cmsid.c_str(),devid.c_str(),token.c_str());
				return 0;
			}

			std::string token = pRoom->get_token();
			Json::Value resp;
                        resp["code"]=101;
                        resp["result"]="success";
                        resp["token"]=token;
                        resp["num"]=count;
			string strresp = resp.toStyledString();
               		this->send(socketid, strresp);
                        this->setValue(socketid,"roomid",cmsid+devid);
			unsigned long cur = Global::GetTickCount();
                        this->updateTimestamp(socketid,cur);
			LOG_INFO("[wss]: id(%d) join @ cmsid(%s) devid(%s)--> success(%s)",socketid,cmsid.c_str(),devid.c_str(),token.c_str());
                        return 0;
		}
		else
		{
			Json::Value resp;
                        resp["code"]=900;
                        resp["result"]="fail,unexcepted situationl!";
                        string strresp = resp.toStyledString();
                	this->send(socketid, strresp);
			LOG_INFO("[wss]: id(%d) @ cmsid(%s) devid(%s)--> fail,unexcepted situationl!",socketid,cmsid.c_str(),devid.c_str());
                        return 0;
		}
	}
	else
	{
		pthread_mutex_unlock(&roomlock_);
		Json::Value resp;
                resp["code"]=900;
                resp["result"]="fail,unexcepted situationl!";
                string strresp = resp.toStyledString();
             	this->send(socketid, strresp);
		LOG_INFO("[wss]: id(%d) @ cmsid(%s) devid(%s)--> fail,unexcepted situationl!",socketid,cmsid.c_str(),devid.c_str());
	}


	return 0;
}

int  PlayMngr::quit_liveroom(int socketid, string cmsid, string devid)
{

	int ret = handler_->check_play_status(cmsid,devid);
	//ret=100:adapter client offline
	//ret=101:device offline 
	//ret=102:device not playing
	//ret=200:device playing
	LOG_INFO("[wss]: id(%d) quit @ cmsid(%s) camera(%s)",socketid,cmsid.c_str(),devid.c_str());	
	LiveRoom *pRoom = NULL;	
	pthread_mutex_lock(&roomlock_);
	std::map<string,LiveRoom*>::iterator iter = rooms_.find(cmsid+devid);
        if( rooms_.end() != iter )
        {
		pRoom = iter->second;
        }

	if(ret==200)
	{
		pthread_mutex_unlock(&roomlock_);
		if(pRoom!=NULL)
		{
			pRoom->quit(socketid);
			this->setValue(socketid,"roomid","");
		}
		
                Json::Value resp;
                resp["code"]=201;
                resp["result"]="stop play success!";
		string strresp = resp.toStyledString();
                this->send(socketid, strresp);
		//LOG_INFO("[wss]: quit play responce->%s",strresp.c_str());
                return 0;

	}
	else
	{
		if(pRoom!=NULL){
			delete pRoom;
			pRoom = NULL;
			rooms_.erase(iter);
		}
		pthread_mutex_unlock(&roomlock_);

		Json::Value resp;
                resp["code"]=201;
                resp["result"]="stop play success!";
		string strresp = resp.toStyledString();
                this->send(socketid, strresp);
		//LOG_INFO("[wss]: quit play responce->%s",strresp.c_str());
                return 0;
	}


	return 0;
}














