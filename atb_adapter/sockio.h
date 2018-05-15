#ifndef sock_io_h_h___
#define sock_io_h_h___


class SOCKIO{
public:

static int ETSend(int fd, char *vptr, int n,int flag)
{
        int  nleft;
        int      nwritten;
        char    *ptr;

        ptr = vptr;
        nleft = n;

        //int nCount = 0;
        while (nleft > 0)
        {
                if ((nwritten = send(fd, ptr, nleft,flag)) <= 0)
                {
                        if (errno == EAGAIN)
			{
				printf("[SOCKET IO]: socket send --> eagain: socket buffer overflow.................\r\n");
				usleep(1000);	
				continue;
			}
                        else
                                return(-1);
                }
                nleft -= nwritten;
                ptr += nwritten;
        }
        return(n - nleft);
}

static int ETRecv(int fd, char *vptr, int n,int flag)
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
			//	continue;
				printf("[SOCKET IO]: socket recv --> eagain: no data to read.................\r\n");
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

};


#endif
