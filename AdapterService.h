#ifndef adapter_service_h_h_h____
#define adapter_service_h_h_h____



#include "PlayMngr.h"



#define MAX_CONN_SIZE        5
#define INITIAL_CONN_SIZE    5
#define HOSTNAME             "xxx.xxx.xxx.xxx"
#define USERNAME             "rxxx"
#define PASSWORD             "xxxxxxxxx"
#define DB_NAME              "xxxxxxx"



class Request;
class Response;
class HttpServer;
class AdapterServer;
class ThreadPool;
class PlayMngr;
class pMysqlpool_;
class AdapterService : public PlayctrlHandler
{
public:
	AdapterService();
	~AdapterService();

public:
	int Start();
	void Stop();


	virtual int check_play_status(string cmsid, string devid);
        virtual int start_play(string cmsid, string devid, string& result, string& token);
        virtual int stop_play(string cmsid, string devid, string& result);


protected:
	static void HandleHls(Request &request, Response &response, long userdata);

	static void HandlePush(Request &request, Response &response, long userdata);
	static void HandleStopPush(Request &request, Response &response, long userdata);

	static void HandleGetCMSClientList(Request &request, Response &response, long userdata);
	static void HandleGetCMSClientInfo(Request &request, Response &response, long userdata);

	static void HandleRegistCMSClient(Request &request, Response &response, long userdata);
	static void HandleWriteoffCMSClient(Request &request, Response &response, long userdata);	
	static void HandleModifyCMSClient(Request &request, Response &response, long userdata);

	static void HandleAddCamera(Request &request, Response &response, long userdata);
	static void HandleDelCamera(Request &request, Response &response, long userdata);
	static void HandleModifyCamera(Request &request, Response &response, long userdata);
	static void HandlePushCheck(Request &request, Response &response, long userdata);



private:
	bool bExit_;

	MysqlConnectionPool *pMysqlpool_;
	ThreadPool    *pThreadpool_;
	HttpServer    *pHttpServer_;	
	AdapterServer *pAdapterServer_;
	PlayMngr      *pPlayMngr_;
};


#endif 
