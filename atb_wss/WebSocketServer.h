/** --------------------------------------------------------------------------
 *  WebSocketServer.h
 *
 *  Base class that WebSocket implementations must inherit from.  Handles the
 *  client connections and calls the child class callbacks for connection
 *  events like onConnect, onMessage, and onDisconnect.
 *
 *  Author    : Jason Kruse <jason@jasonkruse.com> or @mnisjk
 *  Copyright : 2014
 *  License   : BSD (see LICENSE)
 *  -------------------------------------------------------------------------- 
 **/

#ifndef _WEBSOCKETSERVER_H
#define _WEBSOCKETSERVER_H
#include <stdint.h>
#include <map>
#include <deque>
#include <string>
#include <list>
#include <stdio.h>
#include <ctime>
#include <sys/time.h>
#include <iostream>
#include <sstream>
#include "libwebsockets.h"


#include "atb_sak.h"

using namespace std;


#define WS_MAX_DATA_LEN  1024

class BufferString{
    public:
	  BufferString(const char *pstr,int len)
	  {
		memset(p,0,WS_MAX_DATA_LEN);
               if(pstr!=NULL){
		    memcpy(p,pstr,len);
		    length = len;
               }
	  }

	  BufferString(const BufferString &s)
	  {
		if(s.length!=0)
		{
                     memcpy(p,s.p,s.length);
                     this->length = s.length;
		}
	  }

	  BufferString & operator = (const BufferString &s)
	  {
		if(s.length!=0)
		{
                     memcpy(p,s.p,s.length);
                     this->length = s.length;
		}
		return *this;
	  }

	  ~BufferString()
	  {
	  }

	  char *get_buffer(){ return p;}
          int   get_length(){ return length; }

    private:
          char p[WS_MAX_DATA_LEN];
	  int  length;
};
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/// WebSocketServer
/// ---------
class WebSocketServer
{
public:
    // Represents a client connection
    struct Connection
    {
	std::deque<BufferString > buffer;
       // list<string>       buffer;     // Ordered list of pending messages to flush out when socket is writable
	Mutex              locker;
        map<string,string> keyValueMap;
	unsigned long      createTime;
        unsigned long      timestamp;
	int                socketid;
    };

    // Manages connections. Unfortunately this is public because static callback for 
    // libwebsockets is defined outside the instance and needs access to it.
    map<int,Connection*> connections;

    // Constructor / Destructor
    WebSocketServer( int port, const string certPath = "", const string& keyPath = "" );
    ~WebSocketServer( );
    
    void run(       uint64_t timeout = 50     );
    void wait(      uint64_t timeout = 50     );
    void send(      int socketID, string data );
    void broadcast( string data               );
	   

 
    // Key => value storage for each connection
    string getValue( int socketID, const string& name );
    void   setValue( int socketID, const string& name, const string& value );
    int    getNumberOfConnections( );

    void CheckConnectionState(unsigned long cur);
    void updateTimestamp(int socketID, unsigned long timestamp);

    // Overridden by children
    virtual void onConnect(    int socketID                        ) = 0;
    virtual void onMessage(    int socketID, const string& data    ) = 0;
    virtual void onDisconnect( int socketID                        ) = 0;
    virtual void   onError(    int socketID, const string& message ) = 0;
    virtual void onTimeout(int socketid, std::string roomid) = 0;

    // Wrappers, so we can take care of some maintenance
    void onConnectWrapper(    int socketID );
    void onDisconnectWrapper( int socketID );
    void onErrorWrapper( int socketID, const string& message );

protected:
    // Nothing, yet.

    void _removeConnection( int socketID );
private:
    int                  _port;    
    string               _keyPath;
    string               _certPath;
    struct lws_context  *_context;
    Mutex                slocker_;
	 
};

// WebSocketServer.h
#endif
