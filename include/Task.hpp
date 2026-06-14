#ifndef _TASK
#define _TASK

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

enum class TaskState{ READY, RUNNING, BLOCKED, SUSPENDED, WAITING_MUTEX };

struct TaskControlBlock{

    // Idintifiers
    uint64_t id = 0;
    std::string name = "no_name";
    // State
    TaskState state = TaskState::BLOCKED;
    uint8_t priority = 255;                  // 0 -> high, 255 -> low
    uint8_t dynamicPriority = 255;
    // Timing
    uint64_t period = 0;
    uint64_t nextWakeTime = 0;
    uint64_t deadline = 0;
    // Payload
    void (*taskFunction)(void) = nullptr;
    std::thread osThread;
};

#endif