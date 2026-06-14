#ifndef _SCHEDULER
#define _SCHEDULER

#include <atomic>
#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "Task.hpp"

class Scheduler {
private:
    std::vector<std::unique_ptr<TaskControlBlock>> taskList;

    // Needed to simulate the context switch
    std::mutex schedulerMutex;
    std::condition_variable contextSwitchCond;

    // ID of task currently running (-1 if none)
    uint64_t currentRunningTaskId = -1;

    std::atomic<int> taskCounter = 0;

    int ReturnAndIncreaseTaskCounter();
    void TaskExecutionLoop();
    

public:
    bool AddTask(void (*func)(void), const std::string& name, uint8_t priority, uint64_t period);
    void Start();
    void UpdateAndSchedule();
    void Yield();

    TaskControlBlock* GetTaskByThreadId(std::thread::id threadId = std::thread::id());
    
    // Getters
    std::mutex& GetMutex() {
        return schedulerMutex;
    }
    std::condition_variable& GetConditionVariable(){
        return contextSwitchCond;
    }

};

#endif