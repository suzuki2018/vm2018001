#ifndef cms_client_h_h_h__
#define cms_client_h_h_h__

#include <string>
#include <string.h>
#include <list>
#include <map>


	class DeviceNode
	{
	public:
		DeviceNode(){
			state_ = 1;
			allday_ = 1;
		}
		~DeviceNode(){}

		std::string pid_;
		std::string name_;
		std::string token_;//for play
		int state_;// 0-off line 1-online 2-pushing
		int allday_;
		std::string s0_;
		std::string e0_;
		std::string s1_;
		std::string e1_;
		std::string key_;//for push check
	};
	
	typedef std::map<std::string, DeviceNode *> DEVLIST;
	typedef DEVLIST::iterator DEVLISTITOR;
	//use to handle  S -> C message feedback
	#define MAX_BUF_SIZE 10*1024
	class MessageNode
	{
	public:
		MessageNode(){
			memset(buf_,0,MAX_BUF_SIZE);
			timestamp_ = 0;		
			iLength_ = 0;
			seqnum_ = 0;	
		}

		~MessageNode()
		{

		}

		int iType_; //0-get device list  1-push stream
	public:
		unsigned long timestamp_;	
		int seqnum_;
		char buf_[MAX_BUF_SIZE];
		int iLength_;
		//char *pBuffer_;
	};
	typedef std::list<MessageNode *> MSGLIST;
	typedef MSGLIST::iterator MSGLISTITOR;


class TcpServer;
class CMSClient
{
public:
	CMSClient(int fd, TcpServer* pServer);
	~CMSClient();

public:
	//handle http service c -> s
	int GetDeviceList(std::string& result);
	int NotifyPush(std::string devid, std::string& result, std::string& token);
	int NotifyStopPush(std::string devid, std::string& result);
	int GetState(std::string pid);
	
	//handle adapter service c -> s
	void update_livecount(int seq);
	void HandleUploadDevs(char *buf, int size);
	void HandleUpdateChange(char *buf, int size);
	

	int  get_fd(){ return fd_; }
	void set_cmsid(std::string cmsid){ cmsid_ = cmsid;}
	std::string get_cmsid(){ return cmsid_; }

	//use for server to manager client state
	bool CheckState();

	int InsertMsg(int iType, int seqnum, char *buf, int bufsize);
	//bWhenrunning = true  : adapter client login state,notify client add device
	//int AddCamera(std::string pid, std::string name_,bool bWhenrunning=false);
	int AddCamera(std::string pid, std::string name_,int allday, std::string s0, std::string e0, std::string s1, std::string e1, bool bWhenrunning=false);
	int DelCamera(std::string pid);
	int ModifyCamera(std::string pid,int allday, std::string s0, std::string e0, std::string s1, std::string e1);

	bool CheckPush(std::string pid, std::string key);
protected:
	int WaitforResponse(int iType, int seqnum, char *buf, unsigned long timeout);
	void ClearMsglist();
	void ClearDevlist();
private:
	int fd_;
	TcpServer *pServer_;
	std::string cmsid_;
	

	unsigned long livecount_;

	DEVLIST devlist_;
	pthread_mutex_t devlock_;

	MSGLIST msglist_;
	pthread_mutex_t lock_;
};



#endif
