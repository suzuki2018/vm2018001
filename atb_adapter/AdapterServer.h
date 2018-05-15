#ifndef adapter_server_h__
#define adapter_server_h__

#include <string>

class TcpServer;
class ThreadPool;
class MysqlConnectionPool;
class AdapterServer
{
public:
	AdapterServer(ThreadPool *pThreadPool,MysqlConnectionPool *pMysqlpool);
	~AdapterServer();

public:
	int Start(char *host, int port);
	void Stop();

	int start_sync();
	
	//notify client
	int GetDeviceList(std::string cmsid, std::string& result);
	int NotifyStartPush(std::string cmsid, std::string deviceid, std::string& result, std::string& token);
	int NotifyStopPush(std::string cmsid, std::string deviceid, std::string& result);
	
	//unused
	std::string GetCMSClientList(std::string verify, std::string userid);
	//unused
	std::string GetCMSClientInfo(std::string verify, std::string userid,std::string cmsid);	

	//unused
	std::string RegistCMSClientAccount(std::string verify, std::string userid, std::string cmspsw, std::string name);
	//web notify interface
	std::string WriteoffCMSClientAccount(std::string verify, std::string userid, std::string cmsid);
	//unused
	std::string ModifyCMSClientAccount(std::string verify, std::string userid, std::string cmsid, int type, std::string value);
	
	//web + pcclient interface
	std::string AddCamera(std::string verify, std::string userid, std::string cmsid, std::string pid, std::string name, int allday, std::string time_sec0, std::string time_sec1,std::string broadcast_address,bool fromweb=true);
	std::string DelCamera(std::string verify, std::string userid, std::string cmsid, std::string pid,bool fromweb=true);
	std::string ModifyCamera(std::string verify, std::string userid, std::string cmsid, std::string pid, std::string name, int allday,std::string sec0, std::string sec1,bool fromweb=true);
	
	std::string CheckPush(std::string cmsid, std::string devid, std::string key);


	int  CheckStatus(std::string cmsid, std::string devid);
private:
	bool bExit_;

	MysqlConnectionPool *pMysqlpool_;
	ThreadPool *pThreadpool_;
	TcpServer *pServer_;

	std::string host_;
	int port_;
	pthread_t pid_;
};





#endif
