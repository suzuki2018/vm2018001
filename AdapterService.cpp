#include "AdapterService.h"
#include <stdio.h>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/sysinfo.h>

#include "atb_sak.h"

#include "http_server.h"
#include "AdapterServer.h"
#include "server_tcpsrv.h"

AdapterService::AdapterService()
{

        bExit_ = true;
	pThreadpool_ = NULL;
        pHttpServer_ = NULL;
        pAdapterServer_ = NULL;
        pPlayMngr_ = NULL;
	pMysqlpool_ = NULL;
}

AdapterService::~AdapterService()
{
	this->Stop();

}

int AdapterService::check_play_status(string cmsid, string devid)
{
	if(pAdapterServer_==NULL) return -1;
	return pAdapterServer_->CheckStatus(cmsid, devid);
}

int AdapterService::start_play(string cmsid, string devid, string& result, string& token)
{
	if(pAdapterServer_==NULL) 
	{
		result="system quit!";
		return -1;
	}

    	return pAdapterServer_->NotifyStartPush(cmsid, devid, result, token);
}

int AdapterService::stop_play(string cmsid, string devid, string& result)
{
	if(pAdapterServer_==NULL) 
	{
		result="system quit!";
		return -1;
	}
    	return pAdapterServer_->NotifyStopPush(cmsid, devid, result);
}



void AdapterService::HandleGetCMSClientList(Request &request, Response &response, long userdata)
{
    AdapterService *pthis = (AdapterService *)userdata;
    if(pthis == NULL) return;

    Json::Value root;
    std::string str_userid = request.get_param("kid");

    root = pthis->pAdapterServer_->GetCMSClientList("",str_userid);
    response.set_body(root);
}

void AdapterService::HandleGetCMSClientInfo(Request &request, Response &response, long userdata)
{
    AdapterService *pthis = (AdapterService *)userdata;
    if(pthis == NULL) return;

    Json::Value root;
    std::string str_userid = request.get_param("kid");
    std::string str_cmsid = request.get_param("cmsid");

    root = pthis->pAdapterServer_->GetCMSClientInfo("",str_userid, str_cmsid);
    response.set_body(root);
}


void AdapterService::HandlePush(Request &request, Response &response, long userdata) {
    
    AdapterService *pthis = (AdapterService *)userdata;
    if(pthis == NULL) return;

    Json::Value root;
    std::string str_cmsid = request.get_param("cmsid");
    std::string str_devid = request.get_param("devid");
    if (str_cmsid.empty()
	|| str_devid.empty()) {
	root["code"] = 200;
        root["result"] = "param invalid!";
        response.set_body(root);
        return;
    }

    std::string result;
    std::string token;
    int ret = pthis->pAdapterServer_->NotifyStartPush(str_cmsid, str_devid, result, token);
    if(ret!=0)
    {
	root["code"] = 200;
	root["result"] = result;
    	response.set_body(root);
	return;
    }
    
    root["code"] = 100;
    root["result"] = result;	
    root["token"] = token;
    response.set_body(root);
}

void AdapterService::HandleStopPush(Request &request, Response &response, long userdata) {

    AdapterService *pthis = (AdapterService *)userdata;
    if(pthis == NULL) return;

    Json::Value root;
    std::string str_cmsid = request.get_param("cmsid");
    std::string str_devid = request.get_param("devid");
    if (str_cmsid.empty()
        || str_devid.empty()) {
        root["code"] = 200;
        root["result"] = "param invalid!";
        response.set_body(root);
        return;
    }

    std::string result;
    int ret = pthis->pAdapterServer_->NotifyStopPush(str_cmsid, str_devid, result);
    if(ret!=0)
    {
        root["code"] = 200;
        root["result"] = "notify stop push stream fail!";
        response.set_body(root);
        return;
    }

    root["code"] = 100;
    root["result"] = result;
    response.set_body(root);
}

void AdapterService::HandleRegistCMSClient(Request &request, Response &response, long userdata)
{
    AdapterService *pthis = (AdapterService *)userdata;
    if(pthis == NULL) return;

    Json::Value root;
    std::string str_userid = request.get_param("kid");
    std::string str_psw = request.get_param("password");	
    std::string str_name = request.get_param("name");
   // std::string cid = request.get_param("customer_adapter_id"); 

    root = pthis->pAdapterServer_->RegistCMSClientAccount("",str_userid, str_psw, str_name);
    response.set_body(root);
}

void AdapterService::HandleWriteoffCMSClient(Request &request, Response &response, long userdata)
{
    AdapterService *pthis = (AdapterService *)userdata;
    if(pthis == NULL) return;
    
    Json::Value root;
    std::string str_userid = request.get_param("kid");
    std::string str_cmsid = request.get_param("cmsid");  

    root = pthis->pAdapterServer_->WriteoffCMSClientAccount("",str_userid, str_cmsid);
    response.set_body(root);
}

void AdapterService::HandleModifyCMSClient(Request &request, Response &response, long userdata)
{
    AdapterService *pthis = (AdapterService *)userdata;
    if(pthis == NULL) return;

    Json::Value root;
    std::string str_userid = request.get_param("kid");
    std::string str_cmsid = request.get_param("cmsid");
    std::string str_type  = request.get_param("type");
    std::string str_value = request.get_param("value");

    root = pthis->pAdapterServer_->ModifyCMSClientAccount("",str_userid, str_cmsid,atoi(str_type.c_str()),str_value);
    response.set_body(root);
}

void AdapterService::HandleAddCamera(Request &request, Response &response, long userdata)
{
    AdapterService *pthis = (AdapterService *)userdata;
    if(pthis == NULL) return;

    Json::Value root;
    std::string str_clienttype = request.get_param("clienttype");
    std::string str_userid = request.get_param("kid");
    std::string str_cmsid = request.get_param("cmsid");
    std::string str_devid  = request.get_param("devid");
    std::string str_name = request.get_param("name");
    std::string str_allday = request.get_param("allday");
    std::string str_sec0 = request.get_param("time_sec0");
    std::string str_sec1 = request.get_param("time_sec1");
    std::string broadcast_address = request.get_param("broadcast_address");	

    if(str_clienttype=="1")
    {
    	root = pthis->pAdapterServer_->AddCamera("",str_userid, str_cmsid,str_devid,str_name,atoi(str_allday.c_str()),str_sec0, str_sec1,broadcast_address,false);
    	response.set_body(root);
    }
    else
    {
    	pthis->pAdapterServer_->AddCamera("",str_userid, str_cmsid,str_devid,str_name,atoi(str_allday.c_str()),str_sec0, str_sec1,broadcast_address,true);
    }

}

void AdapterService::HandleDelCamera(Request &request, Response &response, long userdata)
{
    AdapterService *pthis = (AdapterService *)userdata;
    if(pthis == NULL) return;

    Json::Value root;
    std::string str_clienttype = request.get_param("clienttype");
    std::string str_userid = request.get_param("kid");
    std::string str_cmsid = request.get_param("cmsid");
    std::string str_devid = request.get_param("devid");

    if(str_clienttype=="1")
    {
    	root = pthis->pAdapterServer_->DelCamera("",str_userid, str_cmsid,str_devid,false);
    	response.set_body(root);
    }
    else
    {
    	pthis->pAdapterServer_->DelCamera("",str_userid, str_cmsid,str_devid,true);
    }
}

void AdapterService::HandleModifyCamera(Request &request, Response &response, long userdata)
{
    AdapterService *pthis = (AdapterService *)userdata;
    if(pthis == NULL) return;

    Json::Value root;
    std::string str_clienttype = request.get_param("clienttype");
    std::string str_userid = request.get_param("kid");
    std::string str_cmsid = request.get_param("cmsid");
    std::string str_devid = request.get_param("devid");
    std::string str_name  = request.get_param("name");
    std::string str_allday = request.get_param("allday");
    std::string sec0 = request.get_param("time_sec0");
    std::string sec1 = request.get_param("time_sec1");

    if(str_clienttype=="1")
    {
    	root = pthis->pAdapterServer_->ModifyCamera("",str_userid, str_cmsid,str_devid,str_name,atoi(str_allday.c_str()),sec0,sec1,false);
    	response.set_body(root);
    }
    else
    {
        pthis->pAdapterServer_->ModifyCamera("",str_userid, str_cmsid,str_devid,str_name,atoi(str_allday.c_str()),sec0,sec1,true);
    }
}

bool CheckPushParser(std::string src, std::string& cmsid, std::string& devid, std::string& key)
{
	int pos1 = src.find("cmsid=");
	if (pos1 == -1) return false;
	std::string tmp = src.substr(pos1+6);
	int pos2 = tmp.find("&devid=");
	if (pos2 == -1) return false;
	cmsid = tmp.substr(0,pos2);
	tmp = tmp.substr(pos2+7);
	int pos3 = tmp.find("&key=");
	if (pos3 == -1) return false;
	devid = tmp.substr(0, pos3);
	key = tmp.substr(pos3+5);

	//LOG_INFO("cmsd:%s\r\ndevid:%s\r\nkey:%s\r\n", cmsid.c_str(), devid.c_str(), key.c_str());
	return true;
}

void AdapterService::HandleHls(Request &request, Response &response, long userdata)
{
    AdapterService *pthis = (AdapterService *)userdata;
    if(pthis == NULL) return;

    RequestBody *pBody = request.get_body();
    std::string strraw = pBody->get_raw_string()->c_str();
    //LOG_INFO("[HTTP REQUEST]: hls request raw string:\r\n%s\r\n", strraw.c_str());
    
    Json::Reader reader;
    Json::Value value;
    if (reader.parse(strraw, value))
    {
        std::string token = value["stream"].asString();
	LOG_INFO("[HTTP REQUEST]: hls notify(%s)",token.c_str());
        pthis->pPlayMngr_->NotifyHls(token);
    }


}

void AdapterService::HandlePushCheck(Request &request, Response &response, long userdata)
{
    AdapterService *pthis = (AdapterService *)userdata;
    if(pthis == NULL) return;

    std::string strRet = "1";

    RequestBody *pBody = request.get_body();
    std::string strraw = pBody->get_raw_string()->c_str();
    //LOG_INFO("[HTTP REQUEST]: push check request raw string:\r\n%s\r\n", strraw.c_str());

    Json::Reader reader;
    Json::Value value;
    if (reader.parse(strraw, value))
    {
        std::string tcUrl = value["tcUrl"].asString();
	LOG_INFO("[HTTP REQUEST]: check push url --> %s",tcUrl.c_str());
	std::string cmsid,devid,key;
	if(CheckPushParser(tcUrl,cmsid,devid,key))
	{
		strRet = pthis->pAdapterServer_->CheckPush(cmsid,devid,key);
	}	
    }
    
    //LOG_INFO("[HTTP REQUEST]: push check request --> strRet:%s\r\n",strRet.c_str());

#if 0
    Json::Value root;
    std::string str_p = request.get_param("tcUrl");
    std::string str_key = request.get_param("key");
    std::string str_cmsid = request.get_param("cmsid");
    std::string str_devid = request.get_param("devid");	

    LOG_INFO("[Http request]: push check %s %s %s\r\n", str_p.c_str(), str_devid.c_str(), str_key.c_str());


    root = pthis->pAdapterServer_->CheckPush(str_cmsid,str_devid,str_key);
    response.set_body(root);
#endif
    response.set_strbody(strRet);
}


int AdapterService::Start()
{
	if(!bExit_) return -1;


	int iRet = 0;
	do{

		int ret = 0;

		pMysqlpool_ = new MysqlConnectionPool();
		ret=pMysqlpool_->initMysqlConnPool(HOSTNAME,3306,USERNAME,PASSWORD,DB_NAME);
		if(ret != 0){
			LOG_INFO("[ASS]: create mysql connection pool fail");
			iRet = -4;
			break;
		}
                ret = pMysqlpool_->openConnPool(INITIAL_CONN_SIZE);
                if(ret != 0) {
			LOG_INFO("[ASS]: create mysql connection pool fail");
			iRet = -4;
			break;
		}

		int core_size = get_nprocs();
		pThreadpool_ = new ThreadPool();
		if(pThreadpool_==NULL) break;
		pThreadpool_->set_pool_size(8);
		if(pThreadpool_->start()!=0)
			break;

		pHttpServer_ = new HttpServer();
		if(pHttpServer_==NULL)
		{
			iRet = -2;
			break;
		}
#if 1
//		pHttpServer_->set_thread_pool(pThreadpool_);

		pHttpServer_->add_mapping("/get_adapterlist",AdapterService::HandleGetCMSClientList,(long )this, GET_METHOD);
		pHttpServer_->add_mapping("/get_cameralist",AdapterService::HandleGetCMSClientInfo,(long )this, GET_METHOD);
//		pHttpServer_->add_mapping("/reg_adapter",AdapterService::HandleRegistCMSClient,(long )this, GET_METHOD);
//		pHttpServer_->add_mapping("/unreg_adapter",AdapterService::HandleWriteoffCMSClient,(long )this, GET_METHOD);
//		pHttpServer_->add_mapping("/mod_adapter",AdapterService::HandleModifyCMSClient,(long )this, GET_METHOD);
		pHttpServer_->add_mapping("/add_camera",AdapterService::HandleAddCamera,(long )this, GET_METHOD);
		pHttpServer_->add_mapping("/del_camera",AdapterService::HandleDelCamera,(long )this, GET_METHOD);
		pHttpServer_->add_mapping("/mod_camera",AdapterService::HandleModifyCamera,(long )this, GET_METHOD);
	//	pHttpServer_->add_mapping("/push",AdapterService::HandlePush,(long )this, GET_METHOD);
	//	pHttpServer_->add_mapping("/stop_push",AdapterService::HandleStopPush,(long )this, GET_METHOD);
		pHttpServer_->add_mapping("/check",AdapterService::HandlePushCheck,(long )this, POST_METHOD);
		pHttpServer_->add_mapping("/hls",AdapterService::HandleHls,(long )this, POST_METHOD);

		pHttpServer_->add_bind_ip("xxx.xxx.xxx.xxx");
		pHttpServer_->set_port(8089);
		pHttpServer_->set_backlog(100000);
       		pHttpServer_->set_max_events(100000);
		ret = pHttpServer_->start_async();
		if(ret != 0)
		{
			LOG_INFO("[ASS]: http server start fail...\r\n");
			iRet = -3;
			break;
		}	
		LOG_INFO("[ASS]: http server start on: 8089\r\n");
#endif
		pAdapterServer_ = new AdapterServer(pThreadpool_,pMysqlpool_);
		if(pAdapterServer_ == NULL)
		{
			iRet = -2;
			break;
		}

		ret = pAdapterServer_->Start("xxx.xxx.xxx.xxx", 8090);
		if(ret != 0)
		{
			iRet = -4;
			break;
		}
		LOG_INFO("[ASS]: adapter server start on:8090\r\n");

		pPlayMngr_ = new PlayMngr(8091);
		if(pPlayMngr_==NULL)
		{
			iRet = -5;
			break;
		}		
		ret = pPlayMngr_->Start(this,pThreadpool_);
		if(ret!=0)
		{
			iRet = -6;
			LOG_INFO("[ASS]: wss fail!\r\n");
			break;
		}
		LOG_INFO("[ASS]: wss service start on:8091\r\n");
		

	}while(0);

	if(iRet<0)
	{
		LOG_INFO("[ASS]: adapter service start fail!\r\n");
		Stop();
		return -1;

	}

	bExit_ = false;

	LOG_INFO("[ASS]: adapter service start....\r\n");
	return 0;
}


void AdapterService::Stop()
{
	LOG_INFO("[ASS]:ass stop...");
	if(pHttpServer_!=NULL)
        {
		pHttpServer_->stop();
                delete pHttpServer_;
                pHttpServer_ = NULL;
        }
	LOG_INFO("[ASS]: http server stop.");

	if(pPlayMngr_!=NULL)
	{
		pPlayMngr_->Stop();
		delete pPlayMngr_;
		pPlayMngr_ = NULL;
	}
	LOG_INFO("[ASS]: wss stop...");

	if(pAdapterServer_!=NULL)
	{
		pAdapterServer_->Stop();
		delete pAdapterServer_;
		pAdapterServer_ = NULL;
	}
	LOG_INFO("[ASS]: adapter server stop.");

	if(pThreadpool_!=NULL)
	{
		delete pThreadpool_;
		pThreadpool_ = NULL;
	}
	LOG_INFO("[ASS]: destroy threadpool stop...");

	if(pMysqlpool_!=NULL)
	{
		delete pMysqlpool_;
		pMysqlpool_ = NULL;
	}
	LOG_INFO("[ASS]: destroy mysql connection pool...");
	LOG_INFO("[ASS]: ass service quit!!!");

	bExit_ = true;
}
