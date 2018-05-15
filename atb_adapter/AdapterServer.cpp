#include "AdapterServer.h"
#include "server_tcpsrv.h"
#include "cms_client.h"
#include "global.h"
#include "db_helper.h"



AdapterServer::AdapterServer(ThreadPool *pThreadPool,MysqlConnectionPool *pMysqlpool)
{

	bExit_ = true;

	port_ = 8090;
	host_ = "127.0.0.1";
	pid_ = 0;

	pMysqlpool_ = pMysqlpool;
	pThreadpool_ = pThreadPool;
	pServer_ = NULL;
}


AdapterServer::~AdapterServer()
{


}

void *adapter_start_routine(void *ptr)
{
	AdapterServer *hs = (AdapterServer *)ptr;
	hs->start_sync();
	return NULL;
}

int AdapterServer::start_sync()
{
	pServer_->Run();
	return 0;
}

int AdapterServer::Start(char *host, int port)
{
	TRACE_MAIN_BEGIN

	if(!bExit_||host==NULL) return -1;

	bool bInit = false;
	int  iRes = 0;
	do{
		DBHelper *pMysql = DBHelper::GetInstance();
		if(pMysql->init(pMysqlpool_)!=0){LOG_INFO("[as]: mysql connection pool init error!"); break; }

		pServer_ = new TcpServer(pThreadpool_);
		if(pServer_==NULL) break;

		if(!pServer_->Connect(host,port)) {
			LOG_INFO("[as]: connect server error");
			break;
		}
		int ret = pthread_create(&pid_, NULL, adapter_start_routine, this );
	        if(ret !=0) break;

		bInit = true;
	}while(0);

	if(!bInit){
		LOG_INFO("[as]: start error!\r\n");
		Stop();

		TRACE_MAIN_LEAVE
		return -2;

	}
	
	host_ = host;
	port_ = port;

	bExit_ = false;

	TRACE_MAIN_LEAVE
	return 0;
}

void AdapterServer::Stop()
{
	if(pServer_!=NULL)
	{
		delete pServer_;
		pServer_ = NULL;
	}

	DBHelper *pMysql = DBHelper::GetInstance();
	pMysql->fint();
	delete pMysql;
	pMysql = NULL;
	
	bExit_ = true;
}

int  AdapterServer::CheckStatus(std::string cmsid, std::string devid)
{
	CMSClient *pClient = pServer_->GetClient(cmsid);
	if(pClient==NULL)
		return 100;// adapter client offline
	
	int istatus = pClient->GetState(devid);
	return (istatus==0)?101:(istatus==1)?102:200;
}


std::string AdapterServer::GetCMSClientList(std::string verify, std::string userid)
{
	std::string strRet;

        if(userid.size()==0)
        {
                strRet = "{\"code\":200,\"result\":\"invalid parameter!\"}";
                return strRet;
        }

	DBHelper *pMysql = DBHelper::GetInstance();
	std::string list;
	int iCount = 0;
	int ret = pMysql->get_user_group(userid,iCount, list);
	//ret: 0-success 1-database err 2-user not exist
        std::string desc;
	if(ret==0)
	     desc = "success!";
	else if(ret==1)
	     desc = "database err!";
	else if(ret==2)
	     desc = "user not exist!";
	else
	     desc = "error known!";

        Json::Value root;
        root["code"] = (ret==0)?100:200;
        root["result"] = desc;
	if(ret==0){
	    root["num"]=iCount;
	    root["list"]=list;
	}
      
        strRet = root.toStyledString();
        return strRet;

}

std::string AdapterServer::GetCMSClientInfo(std::string verify, std::string userid, std::string cmsid)
{
	std::string strRet;

        if(userid.size()==0||cmsid.size()==0)
        {
                strRet = "{\"code\":200,\"result\":\"invalid parameter!\"}";
                return strRet;
        }

	DBHelper *pMysql = DBHelper::GetInstance();
	Json::Value devs;
	int iCount = 0;
	int ret = pMysql->get_group_info(userid, cmsid,iCount, devs);
	//ret: 0-success 1-database err 2-group not exist
        std::string desc;
	if(ret==0)
	     desc = "success!";
	else if(ret==1)
	     desc = "database err!";
	else if(ret==2)
	     desc = "group not exist!";
	else
	     desc = "error known!";

        Json::Value root;
        root["code"] = (ret==0)?100:200;
        root["result"] = desc;
	if(ret==0){
	    root["num"]=iCount;
	    root["list"]=devs;
	}
      
        strRet = root.toStyledString();
        return strRet;
}

int AdapterServer::GetDeviceList(std::string cmsid, std::string& result)
{

	CMSClient *pClient = pServer_->GetClient(cmsid);
	if(pClient == NULL) return -1; //no device found or not on line
	
	return pClient->GetDeviceList(result);
}

int AdapterServer::NotifyStartPush(std::string cmsid, std::string deviceid, std::string& result, std::string& token)
{
	if(cmsid.size()==0|| deviceid.size()==0)
        {
                result  = "{\"code\":200,\"result\":\"invalid parameter!\"}";
                return -1;
        }

	DBHelper *pMysql = DBHelper::GetInstance();
	int ret = pMysql->check_start_push(cmsid, deviceid);
	//ret: 0-check ok 1-database err 2-device not exist 3-camera is not valid at this time
	std::string desc;
	if(ret==0)
	     desc = "success!";
	else if(ret==1)
	     desc = "database err!";
	else if(ret==2)
	     desc = "device not exist!";
	else if(ret==3)
	     desc = "camera is not valid at this time!";
	else
	     desc = "error known!";

	if(ret!=0)
	{
             result = desc;
	     return -1;
	}

	CMSClient *pClient = pServer_->GetClient(cmsid);
        if(pClient == NULL) 
	{
	     result = "start push fail, adapter client offline!";
	     return -1;		
	}

        return pClient->NotifyPush(deviceid, result, token);
}

int AdapterServer::NotifyStopPush(std::string cmsid, std::string deviceid, std::string& result)
{
	if(cmsid.size()==0|| deviceid.size()==0)
        {
                result  = "{\"code\":200,\"result\":\"invalid parameter!\"}";
                return -1;
        }


	CMSClient *pClient = pServer_->GetClient(cmsid);
        if(pClient == NULL) return -1; //no device found or not on line

        return pClient->NotifyStopPush(deviceid, result);
}

std::string AdapterServer::RegistCMSClientAccount(std::string verify, std::string userid, std::string cmspsw, std::string name)
{
	std::string strRet;
	if(userid.size()==0|| cmspsw.size()==0 || name.size()==0)
	{
		strRet = "{\"code\":200,\"result\":\"invalid parameter!\"}";
		return strRet;
	}	

	DBHelper *pMysql = DBHelper::GetInstance();
	std::string groupid;
	int ret = pMysql->add_group(userid, cmspsw, name,groupid);
	std::string desc;
	//ret: 0-success 1-database err 2-group exist 3-reach max num
	if(ret==0)
	     desc = "success!";
	else if(ret==1)
	     desc = "database err!";
	else if(ret==2)
	     desc = "group exist!";
	else if(ret==3)
	     desc = "reach max num,add fail!";
	else
	     desc = "error known!";

	Json::Value root;
        root["code"] = (ret==0)?100:200;
        root["result"] = desc;
	if(ret==0)
            root["cmsid"]= groupid;
        strRet = root.toStyledString();

	return strRet;
}

std::string AdapterServer::WriteoffCMSClientAccount(std::string verify, std::string userid, std::string cmsid)
{
#if 0
	std::string strRet;
	
	if(userid.size()==0|| cmsid.size()==0)
	{
		strRet = "{\"code\":200,\"result\":\"invalid parameter!\"}";
		return strRet;
	}	

	DBHelper *pMysql = DBHelper::GetInstance();
	int ret = pMysql->del_group(userid,cmsid);
	std::string desc;
	//ret: 0-success 1-database err 2-group not exist 3-delete fail
	if(ret==0)
	     desc = "success!";
	else if(ret==1)
	     desc = "database err!";
	else if(ret==2)
	     desc = "group not exist!";
	else if(ret==3)
	     desc = "delete fail!";
	else
	     desc = "error known!";

	if(ret==0)
	{
	     pServer_->DelClient(cmsid);
	}
	
	Json::Value root;
        root["code"] = (ret==0)?100:200;
        root["result"] = desc;
        strRet = root.toStyledString();

	return strRet;
#endif
	pServer_->DelClient(cmsid);
	return "";
}

std::string AdapterServer::ModifyCMSClientAccount(std::string verify, std::string userid, std::string cmsid, int type, std::string value)
{
	std::string strRet;

	if(userid.size()==0|| cmsid.size()==0||value.size()==0
	||(type!=0 && type !=1))
        {
                strRet = "{\"code\":200,\"result\":\"invalid parameter!\"}";
                return strRet;
        }

	DBHelper *pMysql = DBHelper::GetInstance();
	int ret = pMysql->mod_group(cmsid,type,value);
	std::string desc;
	//ret: 0-success 1-database err 2-modify fail
	if(ret==0)
	     desc = "success!";
	else if(ret==1)
	     desc = "database err!";
	else if(ret==2)
	     desc = "modify fail!";
	else
	     desc = "error known!";

	
	Json::Value root;
        root["code"] = (ret==0)?100:200;
        root["result"] = desc;
        strRet = root.toStyledString();


	return strRet;
}

std::string AdapterServer::AddCamera(std::string verify, std::string userid, std::string cmsid, std::string pid, std::string name,int allday, std::string time_sec0, std::string time_sec1,std::string broadcast_address,bool fromweb)
{	
    if(fromweb)
    {
	CMSClient *pClient = pServer_->GetClient(cmsid);
        if(pClient!=NULL)
        {
                std::string starttime1,endtime1,starttime2,endtime2;
                if(allday==0)
                {
                        Global::ParseTimeSection(time_sec0, time_sec1, starttime1,endtime1,starttime2,endtime2);
                }
                pClient->AddCamera(pid, name, allday, starttime1,endtime1,starttime2,endtime2, true);
        }
	return "";
    }

    std::string strRet;
	
    if(userid.size()==0|| cmsid.size()==0||pid.size()==0||name.size()==0)
    {
         strRet = "{\"code\":200,\"result\":\"invalid parameter!\"}";
         return strRet;
    }

    TBDevice info;
    info.pid = pid;
    info.groupid = cmsid;
    info.name = name;
    info.allday = allday;
    info.time_sec0 = time_sec0;
    info.time_sec1 = time_sec1;
    info.broadcast_address = broadcast_address;

    DBHelper *pMysql = DBHelper::GetInstance();
    int ret = pMysql->add_device(info);
    //ret: 0-success 1-database err 2-invalid time schedule parameter 3-already exist 
    std::string desc;
    if(ret==0)
	desc = "success!";
    else if(ret==1)
	desc = "database err!";
    else if(ret==2)
	desc = "invalid time schedule parameter!";
    else if(ret==3)
	desc = "camera already added!";
    else
	desc = "error known!";

    Json::Value root;
    root["code"] = (ret==0)?100:200;
    root["result"] = desc;
    strRet = root.toStyledString();

    if(ret==0)
    {
 	CMSClient *pClient = pServer_->GetClient(cmsid);
	if(pClient!=NULL)
	{
    		std::string starttime1,endtime1,starttime2,endtime2;
    		if(allday==0)
    		{
			Global::ParseTimeSection(time_sec0, time_sec1, starttime1,endtime1,starttime2,endtime2);
   		}
	    	pClient->AddCamera(pid, name, allday, starttime1,endtime1,starttime2,endtime2, true);
	}
    }

    return strRet;
}

std::string AdapterServer::DelCamera(std::string verify, std::string userid, std::string cmsid, std::string pid,bool fromweb)
{
	if(fromweb)
	{
	     CMSClient *pClient = pServer_->GetClient(cmsid);
             if(pClient!=NULL)
                pClient->DelCamera(pid);

	     return "";
	}

	std::string strRet;

	if(userid.size()==0|| cmsid.size()==0||pid.size()==0)
	{
	     strRet = "{\"code\":200,\"result\":\"invalid parameter!\"}";
	     return strRet;
	}	

	DBHelper *pMysql = DBHelper::GetInstance();
	int ret = pMysql->del_device(cmsid,pid);
	std::string desc;
	//ret: 0-success 1-database err 2-camera not exist
	if(ret==0)
	     desc = "success!";
	else if(ret==1)
	     desc = "database err!";
	else if(ret==2)
	     desc = "camera not exist!";
	else
	     desc = "error known!";
		
	Json::Value root;
        root["code"] = (ret==0)?100:200;
        root["result"] = desc;
        strRet = root.toStyledString();

	if(ret==0)
	{
	     CMSClient *pClient = pServer_->GetClient(cmsid);
	     if(pClient!=NULL)
		pClient->DelCamera(pid);
	}

	return strRet;
}

std::string AdapterServer::ModifyCamera(std::string verify, std::string userid, std::string cmsid, std::string pid, std::string name, int allday,std::string sec0, std::string sec1,bool fromweb)
{
    if(fromweb)
    {
	CMSClient *pClient = pServer_->GetClient(cmsid);
        if(pClient!=NULL)
        {
                std::string starttime1,endtime1,starttime2,endtime2;
                if(allday==0)
                {
                        Global::ParseTimeSection(sec0,sec1, starttime1,endtime1,starttime2,endtime2);
                }
                pClient->ModifyCamera(pid, allday, starttime1,endtime1,starttime2,endtime2);
        }
	return "";
    }


    std::string strRet;
    if(userid.size()==0|| cmsid.size()==0||pid.size()==0||name.size()==0||(allday!=0 && allday!=1))
    {
         strRet = "{\"code\":200,\"result\":\"invalid parameter!\"}";
         return strRet;
    }

    TBDevice info;
    info.pid = pid;
    info.groupid = cmsid;
    info.name = name;
    info.allday = allday;
    info.time_sec0 = sec0;
    info.time_sec1 = sec1;

    DBHelper *pMysql = DBHelper::GetInstance();
    int ret = pMysql->mod_device(info);
    //ret: 0-success 1-database err 2-invalid time schedule parameter 3-modify fail
    std::string desc;
    if(ret==0)
	desc = "success!";
    else if(ret==1)
	desc = "database err!";
    else if(ret==2)
	desc = "invalid time schedule parameter!";
    else if(ret==3)
	desc = "modify fail!";
    else
	desc = "error known!";

     
    Json::Value root;
    root["code"] = (ret==0)?100:200;
    root["result"] = desc;
    strRet = root.toStyledString();

    if(ret==0)
    {
	CMSClient *pClient = pServer_->GetClient(cmsid);
	if(pClient!=NULL)
	{
		std::string starttime1,endtime1,starttime2,endtime2;
    		if(allday==0)
    		{
			Global::ParseTimeSection(sec0,sec1, starttime1,endtime1,starttime2,endtime2);
  		}	
	     	pClient->ModifyCamera(pid, allday, starttime1,endtime1,starttime2,endtime2);
	}
    }

    return strRet;	
}

std::string AdapterServer::CheckPush(std::string cmsid, std::string devid, std::string key)
{
	std::string strRet = "1";
	CMSClient *pClient = pServer_->GetClient(cmsid);
	
        if(pClient!=NULL)
	{
		if(pClient->CheckPush(devid, key))
		{
			strRet = "0";
		}

	}

	return strRet;
}
























