#ifndef _TASK_TCP_READ_MSG_H_
#define _TASK_TCP_READ_MSG_H_

#include "atb_sak.h"



class TaskTcpReadMsg : public Task
{
    public:
        TaskTcpReadMsg(void (*fn_ptr)(void*), void* arg);
        ~TaskTcpReadMsg();

        virtual void run();
};

#endif
