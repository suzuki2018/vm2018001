#include "cms_client.h"
#include "server_tcpsrv.h"
#include "json/json.h"

#include "global.h"

#include <unistd.h>

#if 0
#include<time.h> 

unsigned long GetTickCount()  
{  
    struct timespec ts;  
    clock_gettime(CLOCK_MONOTONIC, &ts);  
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);  
} 
#endif

int GeneralSeqnumber()
{
    srand((unsigned)time(0));  
    return rand()%10000;  
}

CMSClient::CMSClient(int fd, TcpServer *pServer):fd_(fd),pServer_(pServer)
{
	livecount_ = Global::GetTickCount();
	pthread_mutex_init(&lock_, NULL);
	pthread_mutex_init(&devlock_, NULL);
}

CMSClient::~CMSClient()
{
	ClearDevlist();
	ClearMsglist();
}

int CMSClient::GetDeviceList(std::string& result)
{
	LOG_INFO("[as]: cmsid(%s) ---> http request to get list",cmsid_.c_str());
	Json::Value root;
        root["code"] = 100;
        Json::Value res;

	pthread_mutex_lock(&devlock_);

	res["num"] = (int )devlist_.size();

	Json::Value devs;
        for(DEVLISTITOR it = devlist_.begin(); it != devlist_.end();it++)
        {
               DeviceNode *p = it->second;
               Json::Value item;
               item["pid"] = p->pid_;
	       item["name"]=p->name_;
	       item["state"] = p->state_;
	       devs.append(item);
        }
	res["list"]=devs;
        root["result"] = res;
        //Json::StyledWriter writer;
       // result= writer.write(root);
	result = root.toStyledString();	

        pthread_mutex_unlock(&devlock_);	

	return 0;
}

int CMSClient::GetState(std::string pid)
{
	int state = 0;
	pthread_mutex_lock(&devlock_);
	
	state = devlist_[pid]->state_;

	pthread_mutex_unlock(&devlock_);
	return state;
}


int  CMSClient::AddCamera(std::string pid, std::string name_, int allday, std::string s0, std::string e0, std::string s1, std::string e1, bool bWhenrunning)
{

	pthread_mutex_lock(&devlock_);
        DeviceNode *p = new DeviceNode();
        p->pid_ = pid;
	p->name_ = name_;
        p->state_ = 0;
	p->allday_ = allday;
	if(allday==0)
	{
		p->s0_ = s0;
		p->e0_ = e0;
		p->s1_ = s1;
  		p->e1_ = e1;
	}

        devlist_[pid] = p;
        pthread_mutex_unlock(&devlock_);


	if(bWhenrunning)
	{
		char sendbuf[1024] = {0};
	        MsgHeader header;
        	memset(&header, 0, sizeof(MsgHeader));
       		strcpy(header.flag, "atb");
        	header.cmd = NOTIFY_DEVICE_CHANGE;
        	int seq = GeneralSeqnumber();
        	header.seq = seq;
        	header.size = sizeof(DeviceListChange);
        	memcpy(sendbuf, (char *)&header, sizeof(MsgHeader));

        	DeviceListChange req;
        	memset(&req, 0, sizeof(DeviceListChange));
        	strcpy(req.pid, pid.c_str());
		req.type=1;
        	memcpy(sendbuf+sizeof(MsgHeader), (char *)&req, sizeof(DeviceListChange));

        	int ret = SOCKIO::ETSend(fd_,sendbuf, sizeof(MsgHeader)+sizeof(DeviceListChange), 0);
        	if(ret != (sizeof(MsgHeader)+sizeof(DeviceListChange)))
        	{
                	pServer_->DelClient(fd_);
                	return -1;
        	}

	}


	return 0;
}

int CMSClient::DelCamera(std::string pid)
{
	bool bFind = false;
	pthread_mutex_lock(&devlock_);
	DEVLISTITOR itor = devlist_.find(pid);
        if (itor != devlist_.end())
        {
             DeviceNode *p = itor->second;
             delete p;
	     p = NULL;
             devlist_.erase(itor);
             bFind = true;
        }
        pthread_mutex_unlock(&devlock_);

	if(!bFind) return -1;

	char sendbuf[1024] = {0};
        MsgHeader header;
        memset(&header, 0, sizeof(MsgHeader));
        strcpy(header.flag, "atb");
        header.cmd = NOTIFY_DEVICE_CHANGE;
        int seq = GeneralSeqnumber();
        header.seq = seq;
        header.size = sizeof(DeviceListChange);
        memcpy(sendbuf, (char *)&header, sizeof(MsgHeader));

        DeviceListChange req;
        memset(&req, 0, sizeof(DeviceListChange));
        strcpy(req.pid, pid.c_str());
        req.type=0;
        memcpy(sendbuf+sizeof(MsgHeader), (char *)&req, sizeof(DeviceListChange));

        int ret = SOCKIO::ETSend(fd_,sendbuf, sizeof(MsgHeader)+sizeof(DeviceListChange), 0);
        if(ret != (sizeof(MsgHeader)+sizeof(DeviceListChange)))
        {
             pServer_->DelClient(fd_);
             return -1;
        }
}

int CMSClient::NotifyPush(std::string devid, std::string& result, std::string& token)
{
	//LOG_INFO("[as]: http request ---------> push(%s)",devid.c_str());
	DeviceNode *pItem = NULL;
	pthread_mutex_lock(&devlock_);

	DEVLISTITOR it = devlist_.find(devid);
	if (it == devlist_.end()) {
		result = "camera id invalid!";
		pthread_mutex_unlock(&devlock_);
		return -1;
	}
	else
	{
		pItem = it->second;
		if (pItem->state_ ==2)//pushing
		{
			result = "success!";
			token = pItem->token_;
			pthread_mutex_unlock(&devlock_);
			return 0;
		}
		else
		{
			
		}
	}

	pthread_mutex_unlock(&devlock_);

	char sendbuf[1024] = {0};
	MsgHeader header;
        memset(&header, 0, sizeof(MsgHeader));
        strcpy(header.flag, "atb");
        header.cmd = REQ_PUSH;
        int seq = GeneralSeqnumber();
        header.seq = seq;
        header.size = sizeof(ReqPush);
	memcpy(sendbuf, (char *)&header, sizeof(MsgHeader));

	ReqPush req;
	memset(&req, 0, sizeof(ReqPush));
	strcpy(req.pid, devid.c_str());
	std::string push_key = Global::GeneralToken(10);
	strcpy(req.key,push_key.c_str());
	memcpy(sendbuf+sizeof(MsgHeader), (char *)&req, sizeof(ReqPush));

	int ret = SOCKIO::ETSend(fd_,sendbuf, sizeof(MsgHeader)+sizeof(ReqPush), 0);
        if(ret != (sizeof(MsgHeader)+sizeof(ReqPush)))
        {
                pServer_->DelClient(fd_);
		result = "network error!";
		return -1;
        }

//	pServer_->ContinueRecv(fd_);

	RespPush resp;
        memset(&resp, 0, sizeof(RespPush));
        ret = WaitforResponse(RESP_PUSH, seq, (char *)&resp, 6000);
        if(ret ==0)
        {	
		if(resp.result==100)
		{
			//LOG_INFO("[CMSClient]: notify start push success ...token(%s) \r\n", resp.token);
			pItem->token_ = resp.token;
			pItem->key_ = push_key;
			pItem->state_ = 2;	
			//LOG_INFO("[CMSClient]: change device node(%s) pushing state:%d token(%s) \r\n",pItem->pid_.c_str(), pItem->state_, pItem->token_.c_str());
			
                        result = "success!";
                        token= pItem->token_;
			return 0;
		}
		else if(resp.result==200)
			result = "camera id invalid!";
		else if(resp.result==300)
			result = "camera offline!";
		else if(resp.result==400)
                        result = "adapter client rtsp module error!";
		else
			result = "unknow error!";

                return -1;
        }

        result = "notify to start push time out!";
        return -1;
}

int CMSClient::NotifyStopPush(std::string devid, std::string& result)
{
	//LOG_INFO("[CMSClient]: http client -> stop push pid(%s)\r\n", devid.c_str());
	DeviceNode *pItem = NULL;
        pthread_mutex_lock(&devlock_);

        DEVLISTITOR it = devlist_.find(devid);
        if (it == devlist_.end()) {
                result = "camera id invalid!";
                pthread_mutex_unlock(&devlock_);
		return -1;
        }
        else
        {
                pItem = it->second;
                if (pItem->state_ ==2)//pushing
                {
                        
                }
                else
                {
			result = "camera is  not in pushing state!";
			pthread_mutex_unlock(&devlock_);
			return -1;
                }
        }

        pthread_mutex_unlock(&devlock_);


	char sendbuf[1024] = {0};
        MsgHeader header;
        memset(&header, 0, sizeof(MsgHeader));
        strcpy(header.flag, "atb");
        header.cmd = REQ_STOP_PUSH;
        int seq = GeneralSeqnumber();
        header.seq = seq;
        header.size = sizeof(ReqStopPush);
        memcpy(sendbuf, (char *)&header, sizeof(MsgHeader));

        ReqStopPush req;
        memset(&req, 0, sizeof(ReqStopPush));
        strcpy(req.pid, devid.c_str());
        memcpy(sendbuf+sizeof(MsgHeader), (char *)&req, sizeof(ReqStopPush));


	int ret = SOCKIO::ETSend(fd_,sendbuf, sizeof(MsgHeader)+sizeof(ReqStopPush), 0);
        if(ret != (sizeof(MsgHeader)+sizeof(ReqStopPush)))
        {
                pServer_->DelClient(fd_);
		result = "adapter client network error!";
                return -1;
        }

        RespStopPush resp;
	memset(&resp, 0, sizeof(RespStopPush));
        ret = WaitforResponse(RESP_STOP_PUSH, seq, (char *)&resp, 6000);
	if(ret ==0)
	{
		//LOG_INFO("[CMCClient]: pid(%s) stop push!\r\n", pItem->pid_.c_str());
		pItem->state_ = 1;
		result = "success!";
		return 0;
	}

	result = "adapter client time out!";
        return -3;

}

void CMSClient::update_livecount(int seq)
{

	livecount_ = Global::GetTickCount();

	MsgHeader header;
        memset(&header, 0, sizeof(MsgHeader));
        strcpy(header.flag, "atb");
        header.cmd = KEEP_ALIVE_RE;
        header.seq = seq;
        header.size = 0;

	int ret = SOCKIO::ETSend(fd_,(char *)&header, sizeof(MsgHeader), 0);
	if(ret != sizeof(MsgHeader))
	{
		LOG_INFO("[as]: cmsid(%s) ---> heartbeat send error!",cmsid_.c_str());
		pServer_->DelClient(fd_);
	}

}

//no use
void CMSClient::HandleUploadDevs(char *buf, int size)
{
	ClearDevlist();

	int num = 0;
	memcpy(&num, buf, 4);

	pthread_mutex_lock(&devlock_);
	DevNode info;
	int i = 0;
	for(i = 0; i<num;i++)
	{
		memcpy(&info, buf+4+sizeof(DevNode)*i, sizeof(DevNode));
		DeviceNode *p = new DeviceNode();
		p->pid_ = info.pid;
		p->token_ = info.token;
		p->state_ = info.state;
	
		devlist_[info.pid] = p;	
	}
	
	LOG_INFO("[C -> S]: device_list .. %d devices online\r\n", i);	

	pthread_mutex_unlock(&devlock_);


}

void CMSClient::HandleUpdateChange(char *buf, int size)
{
	int num = 0;
	memcpy(&num, buf, 4);

	StateItem info;
        int i = 0;
	pthread_mutex_lock(&devlock_);
        for(i = 0; i<num;i++)
        {
                memcpy(&info, buf+4+sizeof(StateItem)*i, sizeof(StateItem));
		
	        DEVLISTITOR itor = devlist_.find(info.pid);
		if (itor != devlist_.end())
		{
			DeviceNode *p = itor->second;
			p->state_ = info.flag;
			
			LOG_INFO("[as]: camera state ---> cmsid(%s) pid(%s) %s", cmsid_.c_str(), info.pid,info.flag==1?"online":"offline");
		}

        }

        pthread_mutex_unlock(&devlock_);
}


#include "global.h"

int CMSClient::InsertMsg(int iType, int seqnum, char *buf, int bufsize)
{
	//std::string curtime = Global::GetCurrentTime();
	//LOG_INFO("Incoming msg: %s\r\n",curtime.c_str()); 

	if( buf==NULL|| bufsize==0) return -1;

	MessageNode *pNode = new MessageNode();
	pNode->iType_ = iType;
//	pNode->pBuffer_ = new char[bufsize];
        memset(pNode->buf_,0,MAX_BUF_SIZE);
	memcpy(pNode->buf_, buf, bufsize);
	pNode->iLength_ = bufsize;
	pNode->seqnum_ = seqnum;
	pNode->timestamp_ = Global::GetTickCount();

	pthread_mutex_lock(&lock_);
	msglist_.push_back(pNode);
	pthread_mutex_unlock(&lock_);

	return 0;
}

int CMSClient::WaitforResponse(int iType, int seqnum, char *buf, unsigned long timeout)
{
	//std::string curtime = Global::GetCurrentTime();
	//LOG_INFO("Begin wait: %s\r\n",curtime.c_str());
	//refresh
	pthread_mutex_lock(&lock_);
	
	for(MSGLISTITOR it = msglist_.begin(); it != msglist_.end();)
	{
		MessageNode *pNode = *it;
		//clear unhandled message
		if(Global::GetTickCount()-pNode->timestamp_>6000)
		{
			delete pNode;
			pNode = NULL;
			it = msglist_.erase(it);
		}
		else
			++it;
		
	}

        pthread_mutex_unlock(&lock_);

	//find
	unsigned long cur = Global::GetTickCount();
	while(1)
	{	
		usleep(40*1000);
		if(Global::GetTickCount()-cur>12*1000)
	//	if(GetTickCount()-cur>timeout)
			return -1;

		pthread_mutex_lock(&lock_);
		for(MSGLISTITOR it = msglist_.begin(); it != msglist_.end();it++)
		{
			MessageNode *pNode = *it;
			if(pNode->iType_==iType
				&& pNode->seqnum_ == seqnum)
			{
				memcpy(buf,pNode->buf_, pNode->iLength_);
				delete pNode;
				pNode = NULL;
				msglist_.erase(it);
				pthread_mutex_unlock(&lock_);
				return 0;
			}
		}
		pthread_mutex_unlock(&lock_);
	}

	return 0;
}

void CMSClient::ClearMsglist()
{
	 pthread_mutex_lock(&lock_);
         for(MSGLISTITOR it = msglist_.begin(); it != msglist_.end();)
         {
		MessageNode *p = *it;
		delete p;
		p = NULL;
		it = msglist_.erase(it);

	 }
	 pthread_mutex_unlock(&lock_);
}

void CMSClient::ClearDevlist()
{
         pthread_mutex_lock(&devlock_);
         for(DEVLISTITOR it = devlist_.begin(); it != devlist_.end();it++)
         {
                DeviceNode *p = it->second;
                delete p;
                p = NULL;
                devlist_.erase(it);

         }
         pthread_mutex_unlock(&devlock_);
}



bool CMSClient::CheckState()
{
	unsigned long cur = Global::GetTickCount();
	unsigned long lv = cur-livecount_;
	
	if(lv>20*1000)
	{
		LOG_INFO("[as]: cmsid(%s) ---> live value time out: %d\r\n", cmsid_.c_str(),lv);
		return false;
	}

	//livecount_ = cur;

	//add 
	int iNum = 0;
	char sendbuf[5*1024] = {0}; 
	std::string strtime = Global::GetCurrentTime(false);
         pthread_mutex_lock(&devlock_);
         for(DEVLISTITOR it = devlist_.begin(); it != devlist_.end();it++)
         {
                DeviceNode *p = it->second;
		if(p->state_ == 2 && p->allday_==0)
		{
			if(p->s0_.size()==0) continue; //data error
			if(Global::CheckInternal(strtime,p->s0_,p->e0_)) continue;	

			if(p->s1_.size()!=0)
                        {
                                if(Global::CheckInternal(strtime,p->s1_,p->e1_))
                                {
                                        continue;
                                }
                        }
			
			memcpy(sendbuf+sizeof(MsgHeader)+iNum*20, p->pid_.c_str(),p->pid_.size());
			iNum++;
			p->state_ = 1;
		}
         }
         pthread_mutex_unlock(&devlock_);


	if(iNum!=0)
	{
		MsgHeader header;
        	memset(&header, 0, sizeof(MsgHeader));
        	strcpy(header.flag, "atb");
        	header.cmd = GROUP_STOP_PUSH;
        	header.seq = 0;
        	header.size = 20*iNum;
		
		memcpy(sendbuf, &header, sizeof(MsgHeader));

        	int ret = SOCKIO::ETSend(fd_,sendbuf, 20*iNum+sizeof(MsgHeader), 0);
        	if(ret != 20*iNum+sizeof(MsgHeader)) return false;
	}

	return true;	
}

int  CMSClient::ModifyCamera(std::string pid,int allday, std::string s0, std::string e0, std::string s1, std::string e1)
{
	 pthread_mutex_lock(&devlock_);

         DEVLISTITOR itor = devlist_.find(pid);
         if (itor != devlist_.end())
         {
         	DeviceNode *p = itor->second;
		p->allday_ = allday;
		p->s0_ = s0;
		p->e0_ = e0;
		p->s1_ = s1;
		p->e1_ = e1;		
        }
        pthread_mutex_unlock(&devlock_);


	return 0;
}

bool CMSClient::CheckPush(std::string pid, std::string key)
{
	pthread_mutex_lock(&devlock_);

        DEVLISTITOR itor = devlist_.find(pid);
        if (itor != devlist_.end())
        {
                DeviceNode *p = itor->second;
        	if(p->key_==key)
		{
//			p->key_="";
       			pthread_mutex_unlock(&devlock_);	
			return true;
		}
	}
        pthread_mutex_unlock(&devlock_);	

	return false;
}




