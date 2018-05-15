#ifndef live_room_h_h_h__
#define live_room_h_h_h__

#include "global.h"
#include "atb_sak.h"
#include <vector>
#include <string>

class LiveRoom
{
public:
	LiveRoom(std::string cmsid, std::string devid, std::string token);
	~LiveRoom();

public:
	int         join(int fd);
	int         quit(int fd);
	std::string get_token() { return token_;}
	void        show_users();
	int         size();
public:
	bool        rdy_;
	std::string cmsid_;
	std::string devid_;
	std::string token_;//play tonken
	std::string id_;//cmsid+devid
	unsigned long timestamp_;//usr for dead count

	std::vector<int> users_;
	Mutex            slocker_;
};


#endif
