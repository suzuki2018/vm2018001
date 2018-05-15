#include "db_helper.h"
#include "global.h"

#include "mysql/mysql_connection_pool.h"

DBHelper::DBHelper()
{
	pool_ = nullptr;
}

DBHelper::~DBHelper()
{
}

int  DBHelper::init(MysqlConnectionPool *pool){

	if(pool_!=nullptr) return -1;
	pool_ = pool;
#if 0
	bool bInit = false;
	do{
		pool_ = new MysqlConnectionPool();
		if(pool_==nullptr) break;
		int ret = pool_->initMysqlConnPool(HOSTNAME, 3306, USERNAME, PASSWORD, DB_NAME);
		if(ret != 0) break;
		ret = pool_->openConnPool(INITIAL_CONN_SIZE);
		if(ret != 0) break;
	
		bInit = true;
	}while(0);

	if(!bInit)
	{
		fint();
		return -1;
	}
#endif

	return 0;
}

int  DBHelper::fint(){
#if 0
	if(pool_==nullptr)
	{
		delete pool_;
		pool_ = nullptr;
	}
#endif
	return 0;
}

// ret: 0-success 1-database error 2-groupid not exit 3-password error
int DBHelper::check_login(std::string groupid, std::string password,std::string& kid){
	if(pool_==nullptr) return 1;
	mysqlConnection *mysqlConn = pool_->fetchConnection();
	if(mysqlConn==nullptr) return 1;
	
	int ret = 0;
	MYSQL_RES *res_ptr = nullptr;
	do{
		char szSql[1024] = {0};
		sprintf(szSql,"select * from %s where cmsid='%s'",TABLE_ADA,groupid.c_str());
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}		
		int num = 0;
		MYSQL_ROW sqlrow;
		res_ptr = mysql_store_result(mysqlConn->sock);
		if(res_ptr) {
			num =  (unsigned long)mysql_num_rows(res_ptr);
			if(num==0) {
				ret = 2;
				break;
			}	
			sqlrow = mysql_fetch_row(res_ptr);
			unsigned long *lengths = mysql_fetch_lengths(res_ptr);
			if(sqlrow[1]==nullptr)
			{
				ret = 100;
				LOG_INFO("[DB]: invalid database data!");
				break;
			}
			if(lengths[1]>19)
			{
				ret = 100;
				LOG_INFO("[DB]: invalid database data!");
				break;
			}
			std::string _psw = std::string(sqlrow[1],lengths[1]);
			if(_psw!=password){
				ret = 3;
				break;
			}
			if(sqlrow[5]==nullptr)
			{
				ret = 100;
				LOG_INFO("[DB]: invalid database data!");
				break;
			}
			kid=std::string(sqlrow[5],lengths[5]);
			
		}
		else
		{
			ret = 1;
			break;
		}
	}while(0);

	if(res_ptr!=nullptr)
		mysql_free_result(res_ptr);
	pool_->recycleConnection(mysqlConn);
	return ret;
}

//ret: 0-check ok 1-database err 2-device not exist 3-camera is not valid at this time
int DBHelper::check_start_push(std::string groupid, std::string pid)
{
	if(pool_==nullptr) return 1;
	mysqlConnection *mysqlConn = pool_->fetchConnection();
	if(mysqlConn==nullptr) return 1;

	int ret = 0;
	MYSQL_RES *res_ptr = nullptr;
	do{
		char szSql[1024] = {0};
		sprintf(szSql,"select * from %s where cmsid='%s' and pid='%s'", TABLE_CAM,groupid.c_str(),pid.c_str());
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}		
		int num = 0;
		MYSQL_ROW sqlrow;
		res_ptr = mysql_store_result(mysqlConn->sock);
		if(res_ptr) {
			num =  (unsigned long)mysql_num_rows(res_ptr);
			if(num==0) {
				ret = 2;
				break;
			}	
			sqlrow = mysql_fetch_row(res_ptr);
			unsigned long *lengths = mysql_fetch_lengths(res_ptr);
			if(sqlrow[4]==nullptr){
				ret = 100;
				LOG_INFO("[DB]: invalid database data,allday value length error!");
				break;
			}
			std::string strallday=std::string(sqlrow[4],lengths[4]);
			if(strallday!="0" && strallday!="1")
			{
				ret=100;
				LOG_INFO("[DB]: invalid database data,allday value not equal 0 or 1!");
				break;
			}

			int iAllday = atoi(strallday.c_str());
			if(iAllday==0)
			{
				std::string strtime = Global::GetCurrentTime(false);
				std::string starttime1 = (sqlrow[5]==nullptr)?"":std::string(sqlrow[5],lengths[5]);
				std::string endtime1 = (sqlrow[6]==nullptr)?"":std::string(sqlrow[6],lengths[6]);
				std::string starttime2 = (sqlrow[7]==nullptr)?"":std::string(sqlrow[7],lengths[7]);
				std::string endtime2 = (sqlrow[8]==nullptr)?"":std::string(sqlrow[8],lengths[8]);
				LOG_INFO("%s-%s %s-%s",(starttime1.size()!=0)?starttime1.c_str():"00:00:00", 
				(endtime1.size()!=0)?endtime1.c_str():"00:00:00",(starttime2.size()!=0)?starttime2.c_str():"00:00:00",(endtime2.size()!=0)?endtime2.c_str():"00:00:00" );	
				if(starttime1.size()==0)
				{
					ret=100;
					LOG_INFO("[DB]: invalid database data,time section 01 invalid!");
					break;
				}
		
				if(!Global::CheckInternal(strtime,starttime1,endtime1))
                		{
					if(starttime2.size()!=0)
					{	
						if(!Global::CheckInternal(strtime,starttime2,endtime2))
	                    			{
							ret=3;
							break;
                        			}
					}
					else
					{
						ret =3;
						break;
					}
				}
			}
		}
		else
		{
			ret = 100;
			LOG_INFO("[DB]: invalid database data!");
			break;
		}
	}while(0);

	if(res_ptr!=nullptr)
		mysql_free_result(res_ptr);
	pool_->recycleConnection(mysqlConn);
	return ret;
}

//ret: 0-success 1-database err 2-user not exist
int DBHelper::get_user_group(std::string kid,int& iCount, std::string& list)
{
	if(pool_==nullptr) return 1;
	mysqlConnection *mysqlConn = pool_->fetchConnection();
	if(mysqlConn==nullptr) return 1;
	
	int ret = 0;
	MYSQL_RES *res_ptr = nullptr;
	MYSQL_ROW sqlrow;
	int iNum = 0;
	Json::Value devs;
	do{
		char szSql[1024]={0};
		std::string strcount;
		sprintf(szSql,"select * from %s where id=%d",TABLE_USERS,atoi(kid.c_str()));
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}	
		
		res_ptr = mysql_store_result(mysqlConn->sock);
		if(res_ptr) {
			int num =  (unsigned long)mysql_num_rows(res_ptr);
			if(num==0) {
				ret = 2;
				break;
			}
		}
		else
		{
			ret = 1;
			break;
		}

		memset(szSql, 0,1024);
        	sprintf(szSql, "select * from %s WHERE user=%d", TABLE_ADA, atoi(kid.c_str()));
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}	
		res_ptr = mysql_store_result(mysqlConn->sock);
		if(res_ptr) {
			int num =  (unsigned long)mysql_num_rows(res_ptr);
			if(num==0) {
				break;
			}
            while((sqlrow = mysql_fetch_row(res_ptr))){
		unsigned long *lengths = mysql_fetch_lengths(res_ptr);
		Json::Value item;
                item["cmsid"] = (sqlrow[0]==nullptr)?"":std::string(sqlrow[0],lengths[0]);
                item["name"]=(sqlrow[2]==nullptr)?"":std::string(sqlrow[2],lengths[2]);
		devs.append(item);
                iNum++;
            }
		}
		else
		{
			ret = 1;
			break;
		}

	}while(0);

	if(ret==0 && iNum>0)
	{
		iCount = iNum;
		list = devs.toStyledString();
	}

	if(res_ptr!=nullptr)
		mysql_free_result(res_ptr);
	pool_->recycleConnection(mysqlConn);
	return ret;
}

int DBHelper::get_group_info_ex(std::string groupid, TBDeviceEx *list)
{
	if(list==nullptr) return -1;
	if(pool_==nullptr) return -1;
	mysqlConnection *mysqlConn = pool_->fetchConnection();
	if(mysqlConn==nullptr) return -1;

	int ret = 0;
	MYSQL_RES *res_ptr = nullptr;
	MYSQL_ROW sqlrow;
	int iNum = 0;
	Json::Value devs;
	do{
		char szSql[1024]={0};
        sprintf(szSql, "select * from %s WHERE cmsid='%s'",TABLE_CAM,groupid.c_str());
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = -1;
			break;
		}	
		res_ptr = mysql_store_result(mysqlConn->sock);
		if(res_ptr) {
			int num =  (unsigned long)mysql_num_rows(res_ptr);
			if(num==0) {
				break;
			}
	
            while((sqlrow = mysql_fetch_row(res_ptr))){
				 unsigned long *lengths = mysql_fetch_lengths(res_ptr);
				if(sqlrow[1]==nullptr||sqlrow[3]==nullptr||sqlrow[4]==nullptr)
				{
					ret = -1;
					LOG_INFO("[DB]: invalid database data!");
					break;
				}

				list[iNum].pid=std::string(sqlrow[1],lengths[1]);
				list[iNum].name=std::string(sqlrow[3],lengths[3]);
				int iAllday=list[iNum].allday=atoi((std::string(sqlrow[4],lengths[4])).c_str());
				if(iAllday==1)
				{

				}
				else
				{
					if(sqlrow[5]==nullptr||sqlrow[6]==nullptr)
					{
						ret = -1;
						LOG_INFO("[DB]: invalid database data!");
						break;
					}
					
					list[iNum].s0=std::string(sqlrow[5],lengths[5]);
					list[iNum].e0=std::string(sqlrow[6],lengths[6]);
					if(sqlrow[7]!=nullptr)
					{
						if(sqlrow[8]==nullptr)
						{
							ret = -1;
							LOG_INFO("[DB]: invalid database data!");
							break;
						}
						list[iNum].s1=std::string(sqlrow[7],lengths[7]);
						list[iNum].e1=std::string(sqlrow[8],lengths[8]);
					}
				}
				
                		iNum++;
				if(iNum>MAX_DEVICE_NUM_PER_ADAPTER) break;
            }
		}
		else
		{
			ret = -1;
			break;
		}

	}while(0);

	ret = iNum;

	if(res_ptr!=nullptr)
		mysql_free_result(res_ptr);
	pool_->recycleConnection(mysqlConn);
	return ret;
}

//ret: 0-success 1-database err 2-group not exist
int DBHelper::get_group_info(std::string kid, std::string groupid, int& iCount, Json::Value& devs)
{
	if(pool_==nullptr) return 1;
	mysqlConnection *mysqlConn = pool_->fetchConnection();
	if(mysqlConn==nullptr) return 1;
	
	int ret = 0;
	MYSQL_RES *res_ptr = nullptr;
	MYSQL_ROW sqlrow;
	int iNum = 0;
	//Json::Value devs;
	do{
		char szSql[1024]={0};
		std::string strcount;
		sprintf(szSql,"select * from %s where cmsid='%s' and user=%d",TABLE_ADA,groupid.c_str(),atoi(kid.c_str()));
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}	
		
		res_ptr = mysql_store_result(mysqlConn->sock);
		if(res_ptr) {
			int num =  (unsigned long)mysql_num_rows(res_ptr);
			if(num==0) {
				ret = 2;
				break;
			}
		}
		else
		{
			ret = 1;
			break;
		}

		memset(szSql, 0,1024);
        	sprintf(szSql, "select * from %s WHERE cmsid='%s'",TABLE_CAM,groupid.c_str());
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}	
		res_ptr = mysql_store_result(mysqlConn->sock);
		if(res_ptr) {
			int num =  (unsigned long)mysql_num_rows(res_ptr);
			if(num==0) {
				break;
			}
            while((sqlrow = mysql_fetch_row(res_ptr))){
		unsigned long *lengths = mysql_fetch_lengths(res_ptr);
		Json::Value item;
                item["devid"] = (sqlrow[1]==nullptr)?"":std::string(sqlrow[1],lengths[1]);
                item["name"]=(sqlrow[3]==nullptr)?"":std::string(sqlrow[3],lengths[3]);
		if(sqlrow[0]==nullptr)
		{
			ret=100;
			LOG_INFO("[DB]: invalid database data!");
			break;	
		}
		item["id"] = atoi(sqlrow[0]);
		int allday=atoi(sqlrow[4]);
		if(allday==1)
		{	
			item["s0"] = "00:00:00";
			item["e0"] = "23:59:59";
			item["s1"] = "";
			item["e1"] = "";
		}
		else{
		item["s0"] = (sqlrow[5]==nullptr)?"":std::string(sqlrow[5],lengths[5]);
		item["e0"] = (sqlrow[6]==nullptr)?"":std::string(sqlrow[6],lengths[6]);
		item["s1"] = (sqlrow[7]==nullptr)?"":std::string(sqlrow[7],lengths[7]);
		item["e1"] = (sqlrow[8]==nullptr)?"":std::string(sqlrow[8],lengths[8]);
		}
		item["addr"] = (sqlrow[11]==nullptr)?"":std::string(sqlrow[11],lengths[11]);
		devs.append(item);
                iNum++;
            }
		}
		else
		{
			ret = 1;
			break;
		}

	}while(0);

	if(ret==0 && iNum>0)
	{
		iCount = iNum;
		//list = devs.toStyledString();
	}

	if(res_ptr!=nullptr)
		mysql_free_result(res_ptr);
	pool_->recycleConnection(mysqlConn);
	return ret;
}

//ret: 0-success 1-database err 2-group exist 3-reach max num
int DBHelper::add_group(std::string kid, std::string password, std::string name,std::string& groupid)
{
	if(pool_==nullptr) return 1;
	mysqlConnection *mysqlConn = pool_->fetchConnection();
	if(mysqlConn==nullptr) return 1;
	
	int ret = 0;
	MYSQL_RES *res_ptr = nullptr;
	MYSQL_ROW sqlrow;
	do{
		char szSql[1024]={0};
		std::string strcount;
		sprintf(szSql,"select * from %s where id=%d", TABLE_USERS,atoi(kid.c_str()));
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}	
		
		res_ptr = mysql_store_result(mysqlConn->sock);
		if(res_ptr) {
			int num =  (unsigned long)mysql_num_rows(res_ptr);
			if(num==0) {
				ret = 2;
				break;
			}

			sqlrow = mysql_fetch_row(res_ptr);
			if(sqlrow[42]==nullptr)
			{
				LOG_INFO("[DB]: invalid database data!");
				ret = 100;
				break;
			}
			strcount = sqlrow[42];
		}
		else
		{
			ret = 1;
			break;
		}

		int iCount = atoi(strcount.c_str());
		if(iCount+1>5)//max num = 5
		{
            		ret = 3;
			break;
		}
		std::string strgroupid = Global::GeneralToken(9);
		std::string strtime = Global::GetCurrentTime();
		memset(szSql, 0, 1024);
		sprintf(szSql,"INSERT INTO %s(cmsid,password,name,datecreate,user)VALUES('%s','%s','%s','%s',%d)",TABLE_ADA,strgroupid.c_str(), password.c_str(), name.c_str(),strtime.c_str(), atoi(kid.c_str()));
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}	

		sprintf(szSql,"UPDATE %s SET adapternum=%d WHERE id=%d",TABLE_USERS,iCount+1,atoi(kid.c_str()));
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}

		groupid = strgroupid;
	}while(0);

	if(res_ptr!=nullptr)
		mysql_free_result(res_ptr);
	pool_->recycleConnection(mysqlConn);
	return ret;
}

//ret: 0-success 1-database err 2-group not exist 3-delete fail
int DBHelper::del_group(std::string kid, std::string groupid)
{
	if(pool_==nullptr) return 1;
	mysqlConnection *mysqlConn = pool_->fetchConnection();
	if(mysqlConn==nullptr) return 1;
	
	int ret = 0;
	MYSQL_RES *res_ptr = nullptr;
	MYSQL_ROW sqlrow;
	do{
		//check exist
		std::string name;
		std::string datecreate;
		char szSql[1024] = {0};
		sprintf(szSql,"select * from %s where cmsid='%s' and user=%d",TABLE_ADA, groupid.c_str(), atoi(kid.c_str()));
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}

		res_ptr = mysql_store_result(mysqlConn->sock);
		if(res_ptr) {
			int num =  (unsigned long)mysql_num_rows(res_ptr);
			if(num==0) {
				ret = 2;
				break;
			}

			sqlrow = mysql_fetch_row(res_ptr);
			unsigned long *lengths = mysql_fetch_lengths(res_ptr);
			if(sqlrow[2]==nullptr||sqlrow[3]==nullptr)
			{
				ret = 100;
				LOG_INFO("[DB]: invalid database data!");
				break;
			}

			name =std::string(sqlrow[2],lengths[2]);
			datecreate=std::string(sqlrow[3],lengths[3]);
		}
		else
		{
			ret = 1;
			break;
		}
		//delete
		memset(szSql, 0,1024);
		sprintf(szSql, "DELETE FROM %s WHERE cmsid='%s'",TABLE_ADA,groupid.c_str());
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}	
		//add to history
		std::string strtime = Global::GetCurrentTime();
		memset(szSql, 0,1024);
		sprintf(szSql, "INSERT INTO %s(cmsid,name,datecreate,datedelete,user)VALUES('%s','%s','%s','%s',%d)", 
			TABLE_ADA_HIS,groupid.c_str(),name.c_str(),datecreate.c_str(),strtime.c_str(),atoi(kid.c_str()));
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}
		//delete all cameras belong to the adapter
        memset(szSql, 0,1024);
        sprintf(szSql, "DELETE FROM %s WHERE cmsid='%s'",TABLE_CAM,groupid.c_str());
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}	
		//update user adapter count
		std::string strcount;
		memset(szSql, 0,1024);
        sprintf(szSql, "select * FROM %s WHERE id=%d",TABLE_USERS,atoi(kid.c_str()));
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}
		res_ptr = mysql_store_result(mysqlConn->sock);
		if(res_ptr) {
			int num =  (unsigned long)mysql_num_rows(res_ptr);
			if(num==0) {
				ret = 100;
				LOG_INFO("[DB]: invalid database data,no specified kid!");
				break;
			}

			sqlrow = mysql_fetch_row(res_ptr);
			if(sqlrow[42]==nullptr)
			{
				LOG_INFO("[DB]: invalid database data,adapternum null!");
				ret = 100;
				break;
			}
			unsigned long *lengths = mysql_fetch_lengths(res_ptr);
			strcount = std::string(sqlrow[42],lengths[42]);
		}
		else
		{
			ret = 1;
			break;
		}

		int iAdapternum = atoi(strcount.c_str());
		if(iAdapternum<=0)
		{
			LOG_INFO("[DB]: invalid database data,invalid adapternum value!");
			ret = 100;
			break;
		}
		memset(szSql, 0,1024);
        	sprintf(szSql, "UPDATE %s SET adapternum=%d WHERE id=%d",TABLE_USERS,iAdapternum-1,atoi(kid.c_str()));
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 2;
			break;
		}
		
	}while(0);

	if(res_ptr!=nullptr)
		mysql_free_result(res_ptr);
	pool_->recycleConnection(mysqlConn);
	return ret;
}

//ret: 0-success 1-database err 2-modify fail
int DBHelper::mod_group(std::string groupid, int type, std::string value)
{
	if(pool_==nullptr) return 1;
	mysqlConnection *mysqlConn = pool_->fetchConnection();
	if(mysqlConn==nullptr) return 1;
	
	int ret = 0;
	do{
		char szSql[1024] = {0};
		std::string curtime = Global::GetCurrentTime();
		if(type==0)
            sprintf(szSql,"UPDATE %s SET password='%s',datemodify='%s' WHERE cmsid='%s'",TABLE_ADA,value.c_str(),curtime.c_str(),groupid.c_str());
		else if(type==1)	
            sprintf(szSql,"UPDATE %s SET name='%s',datemodify='%s' WHERE cmsid='%s'",TABLE_ADA,value.c_str(),curtime.c_str(),groupid.c_str());	

		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 2;
			break;
		}		
		
	}while(0);

	pool_->recycleConnection(mysqlConn);
	return ret;
}

//ret: 0-success 1-database err 2-invalid time schedule parameter 3-already exist 
int DBHelper::add_device(const TBDevice& info)
{
	int iSize = 0;
    std::string starttime1,endtime1,starttime2,endtime2;
    if(info.allday==0)
    {
		int iNum = Global::ParseTimeSection(info.time_sec0, info.time_sec1, starttime1,endtime1,starttime2,endtime2);
		if(iNum==-1) return 2;
        iSize = iNum;
   }	

	if(pool_==nullptr) return 1;
	mysqlConnection *mysqlConn = pool_->fetchConnection();
	if(mysqlConn==nullptr) return 1;
	
	int ret = 0;
	MYSQL_RES *res_ptr = nullptr;
	do{
		//check exist
		char szSql[1024] = {0};
		sprintf(szSql,"select * from %s where pid='%s' and cmsid='%s'",TABLE_CAM,info.pid.c_str(),info.groupid.c_str());
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}		
		int num = 0;
		MYSQL_ROW sqlrow;
		res_ptr = mysql_store_result(mysqlConn->sock);
		if(res_ptr) {
			num =  (unsigned long)mysql_num_rows(res_ptr);
			if(num!=0) {
				ret = 3;
				break;
			}	
		}
		else
		{
			ret = 1;
			break;
		}


		std::string strkid,strcustomerid;
		memset(szSql,0,1024);
		sprintf(szSql,"select * from %s where cmsid='%s'",TABLE_ADA,info.groupid.c_str());
		LOG_INFO("[DB]: %s",szSql);
		
		if(pool_->executeSql(mysqlConn,szSql)!=0)
		{
			ret = 1;
			break;
		}

                res_ptr = mysql_store_result(mysqlConn->sock);
                if(res_ptr) {
                        num =  (unsigned long)mysql_num_rows(res_ptr);
                        if(num==0) {
				LOG_INFO("[DB]: group(%s) not exist!",info.groupid.c_str());
                                ret = 100;
                                break;
                        }
                        sqlrow = mysql_fetch_row(res_ptr);
                        if(sqlrow[5]==nullptr || sqlrow[6]==nullptr)
                        {
                                LOG_INFO("[DB]: invalid database data,invalid kid & customer id!");
                                ret = 100;
                                break;
                        }
                        unsigned long *lengths = mysql_fetch_lengths(res_ptr);
                        strkid = std::string(sqlrow[5],lengths[5]);
                        strcustomerid = std::string(sqlrow[6],lengths[6]);
                }
                else
                {
                        ret = 1;
                        break;
                }
		memset(szSql, 0, 1024);
		std::string strtime = Global::GetCurrentTime();
		if(info.allday==0)
		{
			if(iSize==2)
				sprintf(szSql, "INSERT INTO %s(pid,cmsid,name,allday,starttime1,endtime1,starttime2,endtime2,datecreate,customer_id,kindergarten_id)VALUES('%s','%s','%s',0,'%s','%s','%s','%s','%s',%d,%d)",
					TABLE_CAM,info.pid.c_str(), info.groupid.c_str(),info.name.c_str(),
					starttime1.c_str(),endtime1.c_str(),starttime2.c_str(),endtime2.c_str(),strtime.c_str(),
					atoi(strcustomerid.c_str()), atoi(strkid.c_str()));
			else	
				sprintf(szSql, "INSERT INTO %s(pid,cmsid,name,allday,starttime1,endtime1,datecreate,customer_id,kindergarten_id)VALUES('%s','%s','%s',0,'%s','%s','%s',%d,%d)",
					TABLE_CAM,info.pid.c_str(), info.groupid.c_str(),info.name.c_str(),
					starttime1.c_str(),endtime1.c_str(),strtime.c_str(),
					atoi(strcustomerid.c_str()), atoi(strkid.c_str()));
		}
		else{	
				sprintf(szSql, "INSERT INTO %s(pid,cmsid,name,allday,datecreate,customer_id,kindergarten_id)VALUES('%s','%s','%s',1,'%s',%d,%d)",
					TABLE_CAM,info.pid.c_str(), info.groupid.c_str(),info.name.c_str(),strtime.c_str(),atoi(strcustomerid.c_str()), atoi(strkid.c_str()));
		}	
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}	

	}while(0);

	if(res_ptr!=nullptr)
		mysql_free_result(res_ptr);
	pool_->recycleConnection(mysqlConn);
	return ret;
}

//ret: 0-success 1-database err 2-camera not exist
int DBHelper::del_device(std::string groupid, std::string pid)
{
	if(pool_==nullptr) return 1;
	mysqlConnection *mysqlConn = pool_->fetchConnection();
	if(mysqlConn==nullptr) return 1;
	
	int ret = 0;
	MYSQL_RES *res_ptr = nullptr;
	do{
		char szSql[1024] = {0};
		sprintf(szSql,"select * from %s where cmsid='%s' and pid='%s'",TABLE_CAM,groupid.c_str(), pid.c_str());
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}		
		int num = 0;
		MYSQL_ROW sqlrow;
		res_ptr = mysql_store_result(mysqlConn->sock);
		if(res_ptr) {
			num =  (unsigned long)mysql_num_rows(res_ptr);
			if(num==0) {
				ret = 2;
				break;
			}	
			sqlrow = mysql_fetch_row(res_ptr);
                        if(sqlrow[3]==nullptr || sqlrow[9]==nullptr)
                        {
                                LOG_INFO("[DB]: invalid database data,invalid camera name & createtime!");
                                ret = 100;
                                break;
                        }
                        unsigned long *lengths = mysql_fetch_lengths(res_ptr);
                        std::string strname = std::string(sqlrow[3],lengths[3]);
			std::string strcreate = std::string(sqlrow[9],lengths[9]);

			memset(szSql, 0,1024);
            		sprintf(szSql, "DELETE FROM %s WHERE cmsid='%s' and pid='%s'", TABLE_CAM,groupid.c_str(),pid.c_str());
			if(pool_->executeSql(mysqlConn, szSql)!=0){
				ret = 1;
				break;
			}

			std::string strtime = Global::GetCurrentTime();
                memset(szSql, 0,1024);
                sprintf(szSql, "INSERT INTO %s(pid,cmsid,name,datecreate,datedelete)VALUES('%s','%s','%s','%s','%s')", TABLE_CAM_HIS, pid.c_str(),groupid.c_str(),strname.c_str(),strcreate.c_str(),strtime.c_str());	
		}
		else
		{
			ret = 1;
			break;
		}
	}while(0);

	if(res_ptr!=nullptr)
		mysql_free_result(res_ptr);
	pool_->recycleConnection(mysqlConn);
	return ret;
}

//ret: 0-success 1-database err 2-invalid time schedule parameter 3-modify fail
int DBHelper::mod_device(const TBDevice& info)
{
	int iSize = 0;
    std::string starttime1,endtime1,starttime2,endtime2;
    if(info.allday==0)
    {
		int iNum = Global::ParseTimeSection(info.time_sec0, info.time_sec1, starttime1,endtime1,starttime2,endtime2);
		if(iNum==-1) return 3;
        iSize = iNum;
   }	

	if(pool_==nullptr) return 1;
	mysqlConnection *mysqlConn = pool_->fetchConnection();
	if(mysqlConn==nullptr) return 1;
	
	int ret = 0;
	do{
		//check exist
		char szSql[1024]={0};
		std::string curtime=Global::GetCurrentTime();
		if(info.allday==0)
		{
			if(iSize==2)
			{
				sprintf(szSql,"UPDATE %s SET allday=0,name='%s',starttime1='%s',endtime1='%s',starttime2='%s',endtime2='%s',datemodify='%s' WHERE pid='%s' and cmsid='%s'",
					TABLE_CAM,info.name.c_str(),starttime1.c_str(),endtime1.c_str(),starttime2.c_str(),endtime2.c_str(),curtime.c_str(),info.pid.c_str(),info.groupid.c_str());
			}
			else
			{
				sprintf(szSql,"UPDATE %s SET allday=0,name='%s',starttime1='%s',endtime1='%s',starttime2=null,endtime2=null,datemodify='%s' WHERE pid='%s' and cmsid='%s'",
					TABLE_CAM,info.name.c_str(),starttime1.c_str(),endtime1.c_str(),curtime.c_str(),info.pid.c_str(),info.groupid.c_str());
			}
		}
		else
		{
			sprintf(szSql,"UPDATE %s SET allday=1,name='%s',starttime1=null,endtime1=null,starttime2=null,endtime2=null,datemodify='%s' WHERE pid='%s' and cmsid='%s'",TABLE_CAM,info.name.c_str(),curtime.c_str(),info.pid.c_str(),info.groupid.c_str());
		}
		LOG_INFO("[DB]: %s",szSql);
		if(pool_->executeSql(mysqlConn, szSql)!=0){
			ret = 1;
			break;
		}		
	}while(0);

	pool_->recycleConnection(mysqlConn);
	return ret;
	
}
