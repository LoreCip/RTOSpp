#ifndef _RTOSMUTEX
#define _RTOSMUTEX

#include <atomic>
#include <vector>

#include "Task.hpp"

class RTOSMutex{
private:
    std::atomic<bool> occupied = false;
    TaskControlBlock* owner = nullptr;
    std::vector<TaskControlBlock*> waitingList;

public:
    void Lock();
    void Unlock();

};

#endif