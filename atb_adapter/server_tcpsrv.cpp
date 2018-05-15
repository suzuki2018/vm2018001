#include "server_tcpsrv.h"
#include "utility.h"
#include "cms_client.h"
#include "global.h"
#include "db_helper.h"


TcpServer::TcpServer(ThreadPool* pool)
{
    TRACE_FUNC_BEGIN
    threadPool = pool;

    // epoll descriptor, for handling accept
    epfd = epoll_create(256);
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    // set the descriptor as non-blocking
    setnonblocking(listenfd);
    // event related descriptor
    ev.data.fd = listenfd;
    // monitor in message, edge trigger
    ev.events = EPOLLIN | EPOLLET;
    // register epoll event
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

    pthread_mutex_init(&lock_, NULL);
   // pthread_mutex_init(&sendlock_, NULL);
   
    bExit_ = true;
    bNotify_ = false;

    
    TRACE_FUNC_LEAVE
}

bool  TcpServer::Connect(char *host, uint16_t port)
{
    TRACE_FUNC_BEGIN
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    if (host)
        inet_aton(host, &(serveraddr.sin_addr));
    else
        serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(port);
    int iRet = -1;
    iRet =  bind(listenfd, (sockaddr*)&serveraddr, sizeof(serveraddr));
    if(iRet!=0)
    {
	close(listenfd);
	return false;
    }

    iRet = listen(listenfd, LISTENQ);
    if(iRet==-1)
    {
	close(listenfd);
	return false;
    }
    TRACE_FUNC_LEAVE

    return true;
}

TcpServer::~TcpServer()
{
    TRACE_FUNC_BEGIN
    //delete threadPool;

    Stop();
    TRACE_FUNC_LEAVE
}

void TcpServer::setnonblocking(int sock)
{
    int opts;
    if ((opts = fcntl(sock, F_GETFL)) < 0)
        errexit("GETFL %d failed", sock);
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0)
        errexit("SETFL %d failed", sock);
}

void read_func2(void *data)
{
	LOG_INFO("---------------------------> 2\r\n");
}

void read_func1(void *pdata)
{
    __task_param_read* rdata = (__task_param_read*)pdata;

    int fd = rdata->fd;
    TcpServer *srv = rdata->srv;
    delete rdata;
    rdata = NULL;

    int ret = 0;
    MsgHeader header;
    memset(&header, 0,sizeof(MsgHeader));

    ret = SOCKIO::ETRecv( fd,(char *)&header, sizeof(MsgHeader),0);
    if(ret != sizeof(MsgHeader))
    {
        LOG_INFO("[ReadTask]: fd[%d] read header error!",fd);
        srv->DelClient(fd);
        return;
    }

    char check[4]={0};
    memcpy(check, header.flag, 3);
    if(strcmp(check,"atb")!=0)
    {
        LOG_INFO("[ReadTask]: Invalid header data!");
        srv->DelClient(fd);
        return;
    }

    if(header.size==0)
    {
        srv->ResetOneshot(fd);
        srv->HandlerRead(fd, true, header.cmd,0, NULL, 0);
        return;
    }

    char data[10*1024] = {0};
    ret = SOCKIO::ETRecv(fd,data, header.size,0);
    if(ret != header.size)
    {
        LOG_INFO("[ReadTask]: read data area error");
        srv->DelClient(fd);
        return;
    }

    srv->ResetOneshot(fd);
    srv->HandlerRead(fd, true, header.cmd, header.seq,data, header.size);


}



void TcpServer::Run()
{
	bExit_ = false;
   // TRACE_FUNC_BEGIN
    while(!bExit_)
    {
	//refresh client state  

	pthread_mutex_lock(&lock_);
	for(CLIENTLISTITOR it = clientlist_.begin(); it != clientlist_.end();)
	{
		CMSClient *pClient = *it;
		if(!pClient->CheckState())
		{
			struct epoll_event event;
			int fd = pClient->get_fd();
			event.events = EPOLLIN | EPOLLOUT;
			event.data.fd=fd;
    			epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event);
			close(fd);

			LOG_INFO("[Adatper server]:  delete client by heartbeat................");
			delete pClient;
			pClient = NULL;
			
			it = clientlist_.erase(it);			
		}
		else
			++it;

	}
	pthread_mutex_unlock(&lock_);

        // waiting for epoll event
        nfds = epoll_wait(epfd, events, LISTENQ, TIMEOUT);

        // In case of edge trigger, must go over each event
        for (i = 0; i < nfds; ++i)
        {
            // Get new connection
            if (events[i].data.fd == listenfd)
            {
                // accept the client connection
                connfd = accept(listenfd, (sockaddr*)&clientaddr, &clilen);
                if (connfd < 0)
                    errexit("connfd < 0");
                setnonblocking(connfd);
		LOG_INFO("[Adapter server]: new fd(%d) from %s",connfd,inet_ntoa(clientaddr.sin_addr));
               // echo("[TcpServer] connect from %s \n", inet_ntoa(clientaddr.sin_addr));
                ev.data.fd = connfd;
                // monitor in message, edge trigger
                ev.events = EPOLLIN | EPOLLET;
                // add fd to epoll queue
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
            }
            // Received data
            else if (events[i].events & EPOLLIN)
            {
             //   echo("[TcpServer] got a read request from %d \n", events[i].data.fd);
                if (events[i].data.fd < 0)
                    continue;
             //   LOG_INFO("[TcpServer] put task %d to read queue\n", events[i].data.fd);
#if 1	
                __task_param_read* pkg= new __task_param_read();
                pkg->fd = events[i].data.fd;
                pkg->srv = this;
		
		Task *task = new TaskTcpReadMsg(NULL, pkg);
		int ret = threadPool->add_task(task);
		if (ret != 0) {
			DelClient(events[i].data.fd);
			delete pkg;
			delete task;
		}
#endif

#if 0
		Task *task = new Task(read_func2, NULL);
		if(task==NULL)
		{
			LOG_INFO("############ new error \r\n");
			DelClient(events[i].data.fd);
			continue;
		}
        	int ret = threadPool->add_task(task);
        	if (ret != 0) {

			LOG_INFO("########## add_task error \r\n");
			DelClient(events[i].data.fd);
           		delete task;
			continue;
        	}
		DelClient(events[i].data.fd);
#endif

            }
#if 0
            // Have data to send
            else if (events[i].events & EPOLLOUT)
            {
                if (events[i].data.ptr == NULL)
                    continue;

                if (sendingQueue.size()  == 0)
                    continue;	

		__task_param_write *pData = sendingQueue.front();
		sendingQueue.pop();
		
		Task *task = new TaskTcpWriteMsg(NULL, pData);
		int ret = threadPool->add_task(task);
		if (ret != 0) {
			DelClient(events[i].data.fd);
			delete pData;
			delete task;
		}

            }
#endif
            else
            {
                echo("[TcpServer] Error: unknown epoll event\n");
            }
        }
    }
  //  TRACE_FUNC_LEAVE
}


void TcpServer::Stop()
{
	bExit_ = true;

	pthread_mutex_lock(&lock_);
        for(CLIENTLISTITOR it = clientlist_.begin(); it != clientlist_.end();)
        {
                CMSClient *pClient = *it;
                if(!pClient->CheckState())
                {
                        struct epoll_event event;
                        int fd = pClient->get_fd();
                        event.events = EPOLLIN | EPOLLOUT;
                        event.data.fd=fd;
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event);
                        close(fd);

                        delete pClient;
                        pClient = NULL;

                        it = clientlist_.erase(it);
                }
                else
                        ++it;

        }
        pthread_mutex_unlock(&lock_);




}

bool TcpServer::TriggerSend(int fd, char* msg, int len)
{
 //   TRACE_FUNC_BEGIN
    try
    {
#if 0
        __task_param_write *pkg = new __task_param_write();
        memset(pkg, 0, sizeof(__task_param_write));
	pkg->fd = fd;
        pkg->size = len;
	memcpy(pkg->data, msg, len);
        pkg->srv = this;	
        sendingQueue.push(pkg);
#endif
        // request will be handle in separate threadi
        ContinueSend(fd);
	bNotify_ = true;
     //   ResetOneshot(fd);
    }
    catch(...)
    {
   //     TRACE_FUNC_RET_D(0)
        return false;
    }
   // TRACE_FUNC_RET_D(1)
    return true;
}

void TcpServer::ContinueSend(int fd)
{
    TRACE_FUNC_BEGIN
    // Modify monitored event to EPOLLOUT, wait next loop to send data
    ev.events = EPOLLOUT | EPOLLET;
    // modify moditored fd event
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    TRACE_FUNC_LEAVE
}

void TcpServer::ContinueRecv(int fd)
{
   // TRACE_FUNC_BEGIN
   // echo("[TcpServer] continue to recv.\n");
    // Modify monitored event to EPOLLIN, wait next loop to receive data
    ev.events = EPOLLIN | EPOLLET;
    // modify moditored fd event
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
   // TRACE_FUNC_LEAVE
}

void TcpServer::ResetOneshot(int fd) {
        epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
}

void TcpServer::HandlerRead(int fd, bool result, int cmd, int seq,  char *buf, int size)
{
//	LOG_INFO("Handle read thread[%d]:\r\n",pthread_self());
	CMSClient *pClient = NULL;
	pthread_mutex_lock(&lock_);
        for(CLIENTLISTITOR it = clientlist_.begin(); it != clientlist_.end();it++)
	{
		CMSClient *p = *it;
		if(p->get_fd()==fd)
		{
			pClient = p;
			break;
		}
	}
	pthread_mutex_unlock(&lock_);
	//
	if(pClient==NULL){
		if(cmd!=REQ_LOGIN || size != sizeof(ReqLogin))
		{
			struct epoll_event event;
                	event.events = EPOLLIN | EPOLLOUT;
                	event.data.fd=fd;
                	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event);
                	close(fd);
			return;
		}
		//handle login	
//		LOG_INFO("[Adapter server]:  MSG->Login ..........");
		HandleLoginEx(fd,buf,size,seq);		
		return;
	}
	//handle heartbeat
	if(cmd==KEEP_ALIVE)
	{
//		LOG_INFO("[Adapter server]: fd[%d] MSG->Keepalive ..........\r\n",fd);
		pClient->update_livecount(seq);
		return;
	}
	else if(cmd==UPDATE_CHANGE)
	{
//		LOG_INFO("[Adapter server]: MSG->Update device state..........");
		pClient->HandleUpdateChange(buf, size);
	}
	else
	{
//		std::string cur = Global::GetCurrentTime();
//		LOG_INFO("[Adapter server]: async message[ 0x%x ] %s \r\n", cmd,cur.c_str());
		pClient->InsertMsg(cmd, seq, buf, size);
		return;
	}
	

	return;
}


void TcpServer::HandleLogin(int fd, char *buf, int size, int seq)
{
	ReqLogin req;
	memset(&req, 0, sizeof(ReqLogin));
	memcpy(&req, buf, sizeof(ReqLogin));	
	std::string cmsid = req.cmsid;
	std::string password = req.password;	

	int iResult = 0;
	//check in db
        //..
        

        char sendbuf[1024] = {0};
        MsgHeader header;
        memset(&header, 0, sizeof(MsgHeader));
        strcpy(header.flag, "atb");
        header.cmd = RESP_LOGIN;
        header.seq = seq;
        header.size = sizeof(RespLogin);
        memcpy(sendbuf, (char *)&header, sizeof(MsgHeader));

        RespLogin resp;
        memset(&resp, 0, sizeof(RespLogin));
        resp.result = iResult;
        memcpy(sendbuf+sizeof(MsgHeader), (char *)&resp, sizeof(RespLogin));
	
	int ret = SOCKIO::ETSend(fd, sendbuf, sizeof(MsgHeader)+sizeof(RespLogin),0);
	if(ret != (sizeof(MsgHeader)+sizeof(RespLogin)))
	{
		DelClient(fd);
	}

	if(iResult==0)
        {
               AddClient(fd,cmsid, password);
        }

	return;
}

void TcpServer::HandleLoginEx(int fd, char *buf, int size, int seq)
{
	ReqLogin req;
	memset(&req, 0, sizeof(ReqLogin));
	memcpy(&req, buf, sizeof(ReqLogin));
	std::string cmsid = req.cmsid;
	std::string password = req.password;

	char sendbuf[10*1024] = {0};
	int totalbufsize = 0;
	int iConnected = 0;
	int iCount = 0;
	int ret = 0;
        std::string desc;
	std::string strkid;

	do{

	if(GetClient(cmsid)!=NULL)
	{
	     ret  = 4;//已经登录
	     break;
	}


	DBHelper *pMysql = DBHelper::GetInstance();
	ret = pMysql->check_login(cmsid,password,strkid);
	// ret: 0-success 1-database error 2-groupid not exit 3-password error
	if(ret==0)
	     desc = "success!";
	else if(ret==1)
	     desc = "database err!";
	else if(ret==2)
	     desc = "user not exist!";
	else if(ret==100)
	     desc = "invalid database data!";
	else
	     desc = "error known!";

	if(ret==0)
	{
		TBDeviceEx devlist[MAX_DEVICE_NUM_PER_ADAPTER];
		iCount = pMysql->get_group_info_ex(cmsid,devlist);
		if(iCount<0)
		{
		    iCount = 0;
		    desc = "login success,get device list fail,database err!";
		}
		else
		{
		    CMSClient *pClient = AddClient(fd,cmsid, password);
		    for(int i = 0; i < iCount; i++)
		    {
			pClient->AddCamera(devlist[i].pid, devlist[i].name, devlist[i].allday, devlist[i].s0,devlist[i].e0,devlist[i].s1,devlist[i].e1);
			//LOG_INFO("device[%d].pid: content=%s size=%d",i,(devlist[i].pid).c_str(),(devlist[i].pid).size());
			memcpy(sendbuf+sizeof(MsgHeader)+sizeof(RespLogin)+i*20, (devlist[i].pid).c_str(),(devlist[i].pid).size());
		    }
		}
	}

	}while(0);

	LOG_INFO("[as]: cmsid(%s) login ---> %s",cmsid.c_str(),desc.c_str());

	MsgHeader header;
	memset(&header, 0, sizeof(MsgHeader));
	strcpy(header.flag, "atb");
	header.cmd = RESP_LOGIN;
	header.seq = seq;
	header.size = sizeof(RespLogin)+iCount*20;
	memcpy(sendbuf, (char *)&header, sizeof(MsgHeader));

	RespLogin resp;
	memset(&resp, 0, sizeof(RespLogin));
	resp.result = ret;
	resp.size = iCount;
	if(ret==0)
	{
		strcpy(resp.resv,strkid.c_str());
	}

	memcpy(sendbuf+sizeof(MsgHeader), (char *)&resp, sizeof(RespLogin));
	totalbufsize+=sizeof(RespLogin);

	ret = SOCKIO::ETSend(fd, sendbuf, sizeof(MsgHeader)+sizeof(RespLogin)+iCount*20,0);
	if(ret != (sizeof(MsgHeader)+sizeof(RespLogin)+iCount*20))
	{	
		DelClient(fd);
	}
	return;
}

CMSClient *TcpServer::AddClient(int fd, std::string cmsid, std::string password)
{
	CMSClient *pNode = new CMSClient(fd,this);
	if(pNode==NULL)
	{
		LOG_INFO("[as]: add cilent to memory ---> system error\r\n");
		return NULL;
	}

	pNode->set_cmsid(cmsid);

	pthread_mutex_lock(&lock_);
	clientlist_.push_back(pNode);
	pthread_mutex_unlock(&lock_);
	//LOG_INFO("[as]: add client  ---> cmsid[%s]\r\n", cmsid.c_str());
	return pNode;
}

void TcpServer::DelClient(int fd)
{

	bool bHas = false;
	pthread_mutex_lock(&lock_);
        for(CLIENTLISTITOR it = clientlist_.begin(); it != clientlist_.end();it++)
        {
              CMSClient *pClient = *it;
              if(pClient->get_fd()==fd)
	      {
		   bHas = true;

                   delete pClient;
                   pClient = NULL;
                   clientlist_.erase(it);
		   LOG_INFO("[as]: delete client ---> fd(%d)\r\n", fd);
		   break;
	      }
	}	
	pthread_mutex_unlock(&lock_);

	struct epoll_event event;
        event.events = EPOLLIN | EPOLLOUT;
        event.data.fd=fd;
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event);
        close(fd);

	if(!bHas)
		LOG_INFO("[as]:  delete unknown connection................\r\n");
	return;
}

void TcpServer::DelClient(std::string cmsid)
{
	CMSClient *pClient = NULL;
	int fd = -1;
        pthread_mutex_lock(&lock_);
        for(CLIENTLISTITOR it = clientlist_.begin(); it != clientlist_.end();it++)
        {
              CMSClient *p = *it;
              if(p->get_cmsid()==cmsid)
              {
		   fd = p->get_fd();
                   delete p;
		   p = NULL;
		   clientlist_.erase(it);
                   break;
              }
        }
        pthread_mutex_unlock(&lock_);

	if(fd==-1) return;

	struct epoll_event event;
        event.events = EPOLLIN | EPOLLOUT;
        event.data.fd=fd;
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event);
        close(fd);

        LOG_INFO("[as]: delete client");
}

CMSClient *TcpServer::GetClient(std::string cmsid)
{
	CMSClient *pClient = NULL;
	pthread_mutex_lock(&lock_);
        for(CLIENTLISTITOR it = clientlist_.begin(); it != clientlist_.end();it++)
        {
              CMSClient *p = *it;
              if(p->get_cmsid()==cmsid)
              {
                   pClient = p;
                   break;
              }
        }
        pthread_mutex_unlock(&lock_);
	
	return pClient;
}

void TcpServer::Test(int i)
{
	LOG_INFO("fd ================ %d\r\n",i);
}









