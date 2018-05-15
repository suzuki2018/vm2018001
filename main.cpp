#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "AdapterService.h"

#include <signal.h>
#include <execinfo.h>     /* for backtrace() */  
#define BACKTRACE_SIZE   16


#include "atb_sak.h"

AdapterService *pService = nullptr;


void dump(void)  
{  
    int j, nptrs;  
    void *buffer[BACKTRACE_SIZE];  
    char **strings;  
      
    nptrs = backtrace(buffer, BACKTRACE_SIZE);  
      
    printf("backtrace() returned %d addresses\n", nptrs);  
  
    strings = backtrace_symbols(buffer, nptrs);  
    if (strings == NULL) {  
        perror("backtrace_symbols");  
        exit(EXIT_FAILURE);  
    }  
  
    for (j = 0; j < nptrs; j++)  
        printf("  [%02d] %s\n", j, strings[j]);  
  
    free(strings);  
}


void signal_handler(int signo)  
{  
    printf("\n=========>>>catch signal %d <<<=========\n", signo);  
      
    printf("Dump stack start...\n");  
    dump();  
    printf("Dump stack end...\n");  
  
    signal(signo, SIG_DFL); /* 恢复信号默认处理 */  
    raise(signo);           /* 重新发送信号 */  
}

void sig_handler( int sig)
{
       if(sig == SIGINT){
	
	      if(pService!=nullptr)
	      {
		  pService->Stop();
                  delete pService;
                  pService = nullptr;
	      }

              LOG_INFO("ctrl+c has been keydownd\r\n");
              exit(0);
       }
}


void sig_term_handler(int sig)
{
        LOG_INFO("[sig_term_handler]\r\n");

        if(sig==SIGTERM)
        {
	      if(pService!=nullptr)
              {
                  pService->Stop();
                  delete pService;
                  pService = nullptr;
              }

              LOG_INFO("sigal term ....\r\n");
              exit(0);
        }

}



int main()
{

	signal(SIGSEGV, signal_handler);

	int ret = log_init("./", "atb.conf");
    	if (ret != 0) {
        	printf("log init error!");
        	return 0;
    	}

    	signal(SIGSEGV, signal_handler);
    	signal(SIGINT,  sig_handler);
    	signal(SIGTERM, sig_term_handler);



	char quit[4];
	int i = 0;


#if 1
        pService = new AdapterService();
	if(pService==nullptr)
	{
		LOG_INFO("[AAS]: system error, malloc error!\r\n");
		return -1;
	}

	if(pService->Start()!=0)
	{
		LOG_INFO("[ASS]: ass start error!\r\n");
		pService->Stop();
		delete pService;
		pService = nullptr;
		return -1;
	}

	LOG_INFO("[ASS]: Antuobang video adapter service ...\r\n");
#endif

	while(true)
	{
		if(quit[i & 3]=getchar()=='t')
		{
			if(quit[(i + 1) & 3] == 'q' && quit[(i + 2) & 3] == 'u' && quit[(i + 3) & 3] == 'i')
			{
				break;
			}
		}
		++i;
		usleep(40*1000);
	}


#if 1
	if(pService!=nullptr)
	{
		pService->Stop();
		delete pService;
		pService = nullptr;
	}
#endif

	return 0;
}
