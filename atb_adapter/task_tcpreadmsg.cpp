#include <sys/socket.h>      // recv() and send()
#include <sys/epoll.h>       // epoll interface
#include <unistd.h>
#include <stdio.h>
#include <string.h>          // strerror()
#include <errno.h>

#include "task_tcpreadmsg.h"
#include "server_tcpsrv.h"
#include "utility.h"

#include "msg_def.h"
#include "sockio.h"

#if 0
int ETRecv(int fd, char *vptr, int n,int flag)
{
        int nleft, nread;
        char    *ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0)
        {
                if ((nread = recv(fd, ptr, nleft,flag)) < 0)
                {
                        if (errno == EAGAIN)
                        {
                         
                                break;
                        }

                        
                        break;
                }
                else if (nread == 0)
                {
                        
                        break;
                }

                nleft -= nread;
                ptr += nread;
        }

        return(n - nleft);
}
#endif

TaskTcpReadMsg::TaskTcpReadMsg(void (*fn_ptr)(void*), void* arg) : Task(fn_ptr,arg)
{
    
}

TaskTcpReadMsg::~TaskTcpReadMsg()
{
    
}


void TaskTcpReadMsg::run()
{

#if 1
    __task_param_read* rdata = (__task_param_read*)m_arg;
    
    int fd = rdata->fd;
    TcpServer *srv = rdata->srv;
    delete rdata;
    rdata = NULL;
	
    int ret = 0;
    MsgHeader header;
    memset(&header, 0,sizeof(MsgHeader));
   
    ret = SOCKIO::ETRecv( fd,(char *)&header, sizeof(MsgHeader),0);
    if(ret != sizeof(MsgHeader))
    {
	//printf("[ReadTask]: fd[%d] read header error!\r\n",fd);
	srv->DelClient(fd);
	return;
    }

    char check[4]={0};
    memcpy(check, header.flag, 3);
    if(strcmp(check,"atb")!=0)
    {
	//printf("[ReadTask]: Invalid header data!\r\n");
	srv->DelClient(fd);
        return;
    }

    if(header.size==0)
    {
	srv->ResetOneshot(fd);
        srv->HandlerRead(fd, true, header.cmd,0, NULL, 0);
        return;
    }

    char data[10*1024] = {0};
    ret = SOCKIO::ETRecv(fd,data, header.size,0);
    if(ret != header.size)
    {
	//printf("[ReadTask]: read data area error\r\n");
	srv->DelClient(fd);
        return;
    }
    
    srv->ResetOneshot(fd);
    srv->HandlerRead(fd, true, header.cmd, header.seq,data, header.size);
#endif
}


