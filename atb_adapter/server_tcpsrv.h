#ifndef _SERVER_TCP_SERVER_H_
#define _SERVER_TCP_SERVER_H_

#include <sys/socket.h>      // socket interface
#include <sys/epoll.h>       // epoll interface
#include <netinet/in.h>      // struct sockaddr_in
#include <arpa/inet.h>       // IP addr convertion
#include <fcntl.h>           // File descriptor controller
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>          // bzero()
#include <stdlib.h>          // malloc(), delete 
#include <errno.h>
#include <queue>
#include <string>
#include <list>


#include "utility.h"
#include "atb_sak.h"
#include "task_tcpreadmsg.h"
#include "server.h"

#include "msg_def.h"

#include "sockio.h"

#define OPEN_MAX    100
#define LISTENQ     20
#define INFTIM      1000
#define TIMEOUT     500

#define MAX_DATA_LEN 1024


class CMSClient;
class TcpServer;

// for data transporting
// Don't share it with threads, it's not thread safe
struct __task_param_write
{
    int fd;
    int size;
    char data[MAX_DATA_LEN];
    TcpServer *srv;
};

struct __task_param_read{
	int fd;
	TcpServer*srv;
};

struct __task_param_login
{	
	int fd;
	int seq;
	ReqLogin info;
	TcpServer *srv;
};

struct __task_param_getlist{
	CMSClient *client;
	TcpServer *srv;
};


class TcpServer : public Server
{
    private:
        // message to send
       // pthread_mutex_t sendlock_;
//	std::queue<int >sendingQueue;
        std::queue<__task_param_write*> sendingQueue;
        // epoll descriptor from epoll_create()
        int epfd;
        // register epoll_ctl()
        epoll_event ev;
        // store queued events from epoll_wait()
        epoll_event events[LISTENQ];
        // Set socket as non-blocking
        void setnonblocking(int sock);

        // nfds is number of events (number of returned fd)
        int i, nfds;
        int listenfd, connfd;
        // thread pool
        ThreadPool *threadPool;
        // socket struct
        socklen_t clilen;
        sockaddr_in clientaddr;
        sockaddr_in serveraddr;

	// client manager
	typedef std::list<CMSClient *> CLIENTLIST;
	typedef std::list<CMSClient *>::iterator CLIENTLISTITOR;
	CLIENTLIST clientlist_;
	pthread_mutex_t lock_;

	bool bExit_;
	bool bNotify_;

protected:

//	static void read_func(void *data);
	
    public:
        TcpServer(ThreadPool*);
        virtual ~TcpServer();
        bool Connect(char *host, uint16_t port);
        void Run();
	void Stop();
        bool TriggerSend(int fd, char* data, int len);
        void ContinueSend(int fd);
        void ContinueRecv(int fd);
	void ResetOneshot(int fd);	

	void HandlerRead(int fd, bool result, int cmd, int seq, char *buf, int size);
	CMSClient *AddClient(int fd, std::string cmsid, std::string password);
	void DelClient(int fd);
	void DelClient(std::string cmsid);
	CMSClient *GetClient(std::string cmsid);
	void Test(int i);
	void HandleLogin(int fd, char *buf, int size, int seq);
	void HandleLoginEx(int fd, char *buf, int size, int seq);
	//handle http request
//	void HandleAddCMSClient(std::string user, std::string psw, std::string name, std::string& result);
//	void HandleAddCamera(std::string user, std::string psw, std::string cmsid, std::string pid, std::string name, std::string starttime, std::string endtime);
//	void HandleDelCamera(std::string user, std::string psw, std::string cmsid, std::string pid);
//	void HandleModify
};

#endif
