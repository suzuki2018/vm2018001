#ifndef play_manager_h_h_h______
#define play_manager_h_H_h______



#include "Util.h"
#include "WebSocketServer.h"


class PlayctrlHandler{
public:
	virtual int check_play_status(string cmsid, string devid) = 0;
	virtual int start_play(string cmsid, string devid, string &result, string &token) = 0;	
	virtual int stop_play(string cmsid, string devid, string &result)= 0;

};


class PlayMngr;
struct PMTaskData {
    PlayMngr *pm;
    int socketid;
    string cmdline;
};


class ThreadPool;
class LiveRoom;
/**
 * @ Play control management
 * 
 * */
class PlayMngr:public WebSocketServer
{

public:
	PlayMngr(int port);
	~PlayMngr();

	//websocket server override
	virtual void onConnect(    int socketID                        );
    	virtual void onMessage(    int socketID, const string& data    );
    	virtual void onDisconnect( int socketID                        );
    	virtual void onError(      int socketID, const string& message );
	virtual void onTimeout(int socketid, std::string roomid);
public:
	/* 
 	 * @ start play control management service
	*/
	int  Start(PlayctrlHandler *handler,ThreadPool* threadpool);
	/*
 	 * @ stop play control management service
	*/
	void Stop();


	int  start_sync();

	/*
 	 * @ f:handle client request
	*/
	int  handle_request(int socketid, string cmdline);

        int  NotifyHls(std::string token);
protected:
	int  start_async();


	static void *state_thread(void *ptr);
	int  state_threadfunc();
	
	/*
 	 * @ f:create camera play instance,use the string cmsid+devid as the key of the ,
 	 *     play instance
 	 * @ r:0-success,-1-fail
	*/
	int  join_liveroom(int socketid, string cmsid, string devid);
	/*
 	 * @ f:destroy camera play instance
	*/
	int  quit_liveroom(int socketid, string cmsid, string devid);
private:
	PlayctrlHandler *handler_;
	ThreadPool *threadpool_;

	bool bRunning_;
	pthread_t pid_;

	pthread_t state_pid_;


	//play instances,use cmsid+devic as the key
	map<string,LiveRoom*>rooms_;
	pthread_mutex_t roomlock_;
};





#endif
