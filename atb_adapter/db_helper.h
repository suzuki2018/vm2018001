#ifndef db_helper_h_h_h___
#define db_helper_h_h_h___

#include "atb_sak.h"

#if 0
#define MAX_CONN_SIZE        5
#define INITIAL_CONN_SIZE	 2
#define HOSTNAME             "0.0.0.0"
#define USERNAME             "xxxxxxxxx"
#define PASSWORD             "xxxxxxx"
#define DB_NAME              "xxxxxxxxxx"
#endif

#define TABLE_USERS          "xxxxxxxxxx"
#define TABLE_ADA            "xxxxxxxxxxxxxxxx"
#define TABLE_ADA_HIS        "xxxxxxxxxxxxx"
#define TABLE_CAM            "xxxxxxxxxxxxxxxxxx"
#define TABLE_CAM_HIS        "xxxxxxxxxx"

#define MAX_DEVICE_NUM_PER_ADAPTER	50

class  TBDevice
{
public:
	TBDevice(){
		pid=name=time_sec0=time_sec1=broadcast_address="";
		allday = 0;
	}

public:
	std::string pid;
	std::string groupid;
	std::string name;
	int         allday;
	std::string time_sec0;
	std::string time_sec1;
	std::string broadcast_address;
};

class TBDeviceEx
{
public:
	TBDeviceEx(){
		pid=name=s0=e0=s1=e1="";
		allday=0;
	}

public:
	std::string pid;
	std::string name;
	int         allday;
	std::string s0;
	std::string e0;
	std::string s1;
	std::string e1;
};


class DBHelper : public singleton<DBHelper>
{
public:
	DBHelper();
	~DBHelper();
	
public:
	int  init(MysqlConnectionPool *pool);
	int  fint();

	// ret: 0-success 1-database error 2-groupid not exit 3-password error
	int check_login(std::string groupid, std::string password,std::string& kid);

	int check_start_push(std::string groupid, std::string pid);

	int get_user_group(std::string kid,int& num, std::string& list);

	int get_group_info_ex(std::string groupid, TBDeviceEx *list);
	int get_group_info(std::string kid, std::string groupid,int& num, Json::Value& devs);
	int add_group(std::string kid, std::string password, std::string name, std::string& groupid);
	int del_group(std::string kid, std::string groupid);
	int mod_group(std::string groupid, int type, std::string value);

	
	int add_device(const TBDevice& info);
	int del_device(std::string groupid, std::string pid);
	int mod_device(const TBDevice& info);

private:
	MysqlConnectionPool *pool_;
};


#endif
