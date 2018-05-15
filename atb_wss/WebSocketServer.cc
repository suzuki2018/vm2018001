/** --------------------------------------------------------------------------
 *  WebSocketServer.cpp
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

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include "libwebsockets.h"
#include "Util.h"
#include "WebSocketServer.h"

#include "atb_sak.h"


using namespace std;

// 0 for unlimited
#define MAX_BUFFER_SIZE 0

// Nasty hack because certain callbacks are statically defined
WebSocketServer *self;

static int callback_main(   struct lws *wsi, 
                            enum lws_callback_reasons reason, 
                            void *user, 
                            void *in, 
                            size_t len )
{
    int fd;
    unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 512 + LWS_SEND_BUFFER_POST_PADDING];
    unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
    
    switch( reason ) {
        case LWS_CALLBACK_ESTABLISHED:
            self->onConnectWrapper( lws_get_socket_fd( wsi ));
            lws_callback_on_writable( wsi );
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            fd = lws_get_socket_fd( wsi );
	    
            while( self->connections[fd]!=NULL && !self->connections[fd]->buffer.empty( ) )
            {
		self->connections[fd]->locker.lock();
		BufferString buf = self->connections[fd]->buffer.front();
               // string message = self->connections[fd]->buffer.front();
		self->connections[fd]->buffer.pop_front(); 
		self->connections[fd]->locker.unlock();
                
		int msgLen = buf.get_length();
               // int msgLen = message.length();
               // int charsSent = lws_write( wsi, (unsigned char*)message.c_str(), msgLen, LWS_WRITE_TEXT );
                int charsSent = lws_write( wsi, (unsigned char*)buf.get_buffer(), msgLen, LWS_WRITE_TEXT );
                if( charsSent < msgLen )
                    self->onErrorWrapper( fd, string( "Error writing to socket" ) );
                else
		{
                    // Only pop the message if it was sent successfully.
            //        self->connections[fd]->buffer.pop_front( ); 
		}
            }
            lws_callback_on_writable( wsi );
            break;
        
        case LWS_CALLBACK_RECEIVE:
            self->onMessage( lws_get_socket_fd( wsi ), string( (const char *)in ) );
            break;

        case LWS_CALLBACK_CLOSED:
            self->onDisconnectWrapper( lws_get_socket_fd( wsi ) );
            break;

        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "/",
        callback_main,
        0, // user data struct not used
        MAX_BUFFER_SIZE,
    },{ NULL, NULL, 0, 0 } // terminator
};

WebSocketServer::WebSocketServer( int port, const string certPath, const string& keyPath )
{
    this->_port     = port;
    this->_certPath = certPath;
    this->_keyPath  = keyPath; 

    lws_set_log_level( 0, lwsl_emit_syslog ); // We'll do our own logging, thank you.
    struct lws_context_creation_info info;
    memset( &info, 0, sizeof info );
    info.port = this->_port;
    info.iface = NULL;
    info.protocols = protocols;
#ifndef LWS_NO_EXTENSIONS
    info.extensions = lws_get_internal_extensions( );
#endif
    
    if( !this->_certPath.empty( ) && !this->_keyPath.empty( ) )
    {
        Util::log( "Using SSL certPath=" + this->_certPath + ". keyPath=" + this->_keyPath + "." );
        info.ssl_cert_filepath        = this->_certPath.c_str( );
        info.ssl_private_key_filepath = this->_keyPath.c_str( );
    } 
    else 
    {
        Util::log( "Not using SSL" );
        info.ssl_cert_filepath        = NULL;
        info.ssl_private_key_filepath = NULL;
    }
    info.gid = -1;
    info.uid = -1;
    info.options = 0;

    // keep alive
    //info.ka_time = 60; // 60 seconds until connection is suspicious
    //info.ka_probes = 10; // 10 probes after ^ time
    //info.ka_interval = 10; // 10s interval for sending probes
    info.ka_time = 0; // 60 seconds until connection is suspicious
    info.ka_probes = 0; // 10 probes after ^ time
    info.ka_interval = 0; // 10s interval for sending probes
    this->_context = lws_create_context( &info );
    if( !this->_context )
        throw "libwebsocket init failed";
    Util::log( "Server started on port " + Util::toString( this->_port ) ); 

    // Some of the libwebsocket stuff is define statically outside the class. This 
    // allows us to call instance variables from the outside.  Unfortunately this
    // means some attributes must be public that otherwise would be private. 
    self = this;
}

WebSocketServer::~WebSocketServer( )
{
    slocker_.lock();
    // Free up some memory
    for( map<int,Connection*>::const_iterator it = this->connections.begin( ); it != this->connections.end( ); ++it )
    {
        Connection* c = it->second;
        this->connections.erase( it->first );
        delete c;
    }
    slocker_.unlock();
}

void WebSocketServer::onConnectWrapper( int socketID )
{
    Connection* c = new Connection;
    c->createTime = time( 0 );
    c->socketid = socketID;
    
    slocker_.lock();
    this->connections[ socketID ] = c;
    slocker_.unlock();

    this->onConnect( socketID );
}

void WebSocketServer::onDisconnectWrapper( int socketID )
{
    this->onDisconnect( socketID );
    this->_removeConnection( socketID );
}

void WebSocketServer::onErrorWrapper( int socketID, const string& message )
{
    Util::log( "Error: " + message + " on socketID '" + Util::toString( socketID ) + "'" ); 
    this->onError( socketID, message );
    //this->_removeConnection( socketID );
}

void WebSocketServer::send( int socketID, string data )
{
    slocker_.lock();
    std::map<int, Connection*>::iterator it;
    it  = this->connections.find(socketID);
    if(it==this->connections.end()){
    	 slocker_.unlock();
         return;
    }
    slocker_.unlock();

    Connection *c = it->second;
    // Push this onto the buffer. It will be written out when the socket is writable.
    this->connections[socketID]->locker.lock();
    BufferString buf(data.c_str(), data.size());
    c->buffer.push_back(buf);
    this->connections[socketID]->locker.unlock();
}

void WebSocketServer::broadcast( string data )
{
    for( map<int,Connection*>::const_iterator it = this->connections.begin( ); it != this->connections.end( ); ++it )
        this->send( it->first, data );
}

void WebSocketServer::setValue( int socketID, const string& name, const string& value )
{
    slocker_.lock();
    std::map<int, Connection*>::iterator it;
    it  = this->connections.find(socketID);
    if(it==this->connections.end()){
    	 slocker_.unlock();
         return;
    }
    slocker_.unlock();
    Connection *c = it->second;

    c->keyValueMap[name] = value;
}

void WebSocketServer::CheckConnectionState(unsigned long cur)
{
    slocker_.lock();
    //LOG_INFO("connecton map size: %d",this->connections.size());
    
    map<int,Connection*>::iterator it = this->connections.begin();
    while(it != this->connections.end())
    {
	Connection *c = it->second;
        int socketid = c->socketid;
        std::string roomid = c->keyValueMap["roomid"];

        if(cur-c->timestamp>30*1000 && roomid.size()>0)
	{
	//	lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY,(unsigned char *)"seeya", 5);
                this->onTimeout(socketid,roomid);
		c->keyValueMap["roomid"] = "";
		
	//	delete it->second;
	//	it->second=NULL;
	//	this->connections.erase(it++);
	//	LOG_INFO(" - - - - - - - - - delete one  size=%d",this->connections.size());
	}
	else
	{
		it++;
	}
    }

#if 0
    for( map<int,Connection*>::const_iterator it = this->connections.begin( ); it != this->connections.end( );)
    {
	Connection *c = it->second;
	int socketid = c->socketid;
	std::string roomid = c->keyValueMap["roomid"];
	struct lws* wsi = c->wsi;
	if(cur-c->timestamp>15*1000)
	{
		lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY,(unsigned char *)"seeya", 5);
		this->onDisconnect( c->socketid,roomid);
		delete c;
    		c = NULL;
		it=this->connections.erase(it);		
	}
	else
	{
		it++;
	}
    }
#endif    
    slocker_.unlock();
}

void WebSocketServer::updateTimestamp(int socketID, unsigned long timestamp)
{
    slocker_.lock();
    std::map<int, Connection*>::iterator it;
    it  = this->connections.find(socketID);
    if(it==this->connections.end()){
         slocker_.unlock();
         return;
    }
    slocker_.unlock();
    Connection *c = it->second;
    c->timestamp=timestamp;
    return;
}

string WebSocketServer::getValue( int socketID, const string& name )
{
    slocker_.lock();
    std::map<int, Connection*>::iterator it;
    it  = this->connections.find(socketID);
    if(it==this->connections.end()){
    	 slocker_.unlock();
         return "";
    }
    slocker_.unlock();
    Connection *c = it->second;
    return c->keyValueMap[name];
}
int WebSocketServer::getNumberOfConnections( )
{
    slocker_.lock();
    int size = this->connections.size();
    slocker_.unlock();

    return size;
}

void WebSocketServer::run( uint64_t timeout )
{
    int iCount = 0;
    while( 1 )
    {
        this->wait( timeout );
	usleep(50*1000);
#if 0
	if(++iCount>6000)
	{
		iCount=0;
		LOG_INFO("w...");
	}
#endif
    }
}

void WebSocketServer::wait( uint64_t timeout )
{
    if( lws_service( this->_context, timeout ) < 0 )
    {
	LOG_INFO("[wss]: websockets server exiception!");
	Util::log("wait.........throw");
        throw "Error polling for socket activity.";
    }
    else
    {
//	Util::log("wait.........timeout.");
    }
}

void WebSocketServer::_removeConnection( int socketID )
{
    slocker_.lock();
    std::map<int, Connection*>::iterator it;
    it  = this->connections.find(socketID);
    if(it==this->connections.end()){
    	 slocker_.unlock();
         return;
    }
    Connection *c = it->second;
    delete c;
    c = NULL;
    this->connections.erase(it);
    slocker_.unlock();
    return;
} 
