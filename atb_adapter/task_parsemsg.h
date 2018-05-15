#ifndef message_list_travel_h_h_h___
#define message_list_travel_h_h_h___

#include "task.h"

class TaskTravelList : public TaskEx
{
    public:
        TaskTravelList(void *arg);
        ~TaskTravelList();
        void run();
};



#endif
