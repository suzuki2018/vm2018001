#include "global.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

const int LEN = 62; // 26 + 26 + 10
char g_arrCharElem[LEN] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', \
'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', \
'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };




unsigned long Global::GetTickCount()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

std::string Global::GeneralToken(int len)
{
	std::string strRet;

	char* szStr = new char[len + 1];
	szStr[len] = '\0';
	srand((unsigned)time(0));
	int iRand = 0;
	for (int i = 0; i < len; ++i)
	{
		iRand = rand() % LEN;            // iRand = 0 - 61
		szStr[i] = g_arrCharElem[iRand];
	}

	strRet = szStr;

	delete[] szStr;
	return strRet;
}

std::string Global::GetCurrentTime(bool bWithDate){
	std::string strRet;

	struct tm *t;
 	time_t tt;
 	time(&tt);
 	t = localtime(&tt);
	char szTime[128] = {0};

	if(bWithDate)
		sprintf(szTime, "%04d-%02d-%02d %02d:%02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	else
		sprintf(szTime, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
		
	strRet = szTime;
	return strRet;	
}

bool Global::CheckTimeFormat(std::string strtime)
{
	char fmt[] = "%H:%M:%S";
	struct tm tb;
	if (strptime(strtime.c_str(), fmt, &tb) != NULL) {
		return true;
	}

	return false;
}

int  Global::ParseTimeSection(std::string sec0, std::string sec1, std::string& s0, std::string& e0, std::string& s1, std::string& e1)
{
	int pos = sec0.find("-");
	if (pos==-1) return -1;

	std::string s = sec0.substr(0, pos);
	s.insert(2,":");
	s.insert(5,":");
	std::string e = sec0.substr(pos+1);
	e.insert(2,":");
        e.insert(5,":");
	if(!CheckTimeFormat(s)||!CheckTimeFormat(e)) return -1;
	if(!TimeCompare(s,e)) return -1;
	s0 = s;
	e0 = e;
	
	if(sec1.size()==0) return 1;

	pos = sec1.find("-");
        if (pos==-1) return -1;

        s = sec1.substr(0, pos);
        e = sec1.substr(pos+1);
	s.insert(2,":");
        s.insert(5,":");
	e.insert(2,":");
        e.insert(5,":");
        if(!CheckTimeFormat(s)||!CheckTimeFormat(e)) return -1;
        if(!TimeCompare(s,e)) return -1;
        s1 = s;
        e1 = e;

	if(!TimeCompare(e0,s1)) return -1;

	return 2;
}

bool Global::TimeCompare(std::string starttime,std::string endtime)
{
	if(!CheckTimeFormat(starttime)||!CheckTimeFormat(endtime))
		return false;

	struct tm tmS,tmE;
        time_t tS, tE;
        memset(&tmS, 0, sizeof(tmS));
        memset(&tmE, 0, sizeof(tmE));
        strptime(starttime.c_str(), "%H:%M:%S", &tmS);
        strptime(endtime.c_str(), "%H:%M:%S", &tmE);
        tS = mktime(&tmS);
        tE = mktime(&tmE);

	if(tS>tE) return false;

	return true;
}

bool Global::CheckInternal(std::string strtime, std::string starttime, std::string endtime)
{
	if(starttime.size()==0 && endtime.size()==0) return true;

	if(!TimeCompare(starttime, endtime)) return false;
	if(!TimeCompare(starttime,strtime)) return false;
	if(!TimeCompare(strtime,endtime)) return false;

	return true;
}




















