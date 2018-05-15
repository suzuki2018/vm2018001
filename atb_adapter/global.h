#ifndef global_h_h_h__
#define global_h_h_h__


#include <string>


class Global{

public:
	static unsigned long GetTickCount();
	static std::string GeneralToken(int len);
	static std::string GetCurrentTime(bool bWithDate = true);
	static bool CheckTimeFormat(std::string strtime);
	static bool TimeCompare(std::string starttime,std::string endtime);
	static bool CheckInternal(std::string strtime, std::string starttime, std::string endtime);
	static int  ParseTimeSection(std::string sec0, std::string sec1, std::string& s0, std::string& e0, std::string& s1, std::string& e1);

};












#endif
