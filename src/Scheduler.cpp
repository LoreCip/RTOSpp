#include "Scheduler.hpp"
#include "Task.hpp"
#include "SystemTimer.hpp"

#include "utils.hpp"

std::chrono::time_point<std::chrono::high_resolution_clock> rtosStartTime;

int Scheduler::ReturnAndIncreaseTaskCounter(){
    int old = taskCounter.load();
    taskCounter.fetch_add(1, std::memory_order_relaxed);
    return old;
}

TaskControlBlock* Scheduler::GetTaskByThreadId(std::thread::id threadId) {
    // Se non è stato passato alcun argomento, prendiamo l'ID di chi sta chiamando la funzione
    if (threadId == std::thread::id()) {
        threadId = std::this_thread::get_id();
    }
    
    for (auto &task : taskList) { 
        if (task->osThread.get_id() == threadId) {
            return task.get();
        }
    }
    return nullptr;
}

void Scheduler::TaskExecutionLoop() {
    // Find yourself
    TaskControlBlock* myself = GetTaskByThreadId();

    // Safety check: exit if ID is invalid
    if (myself == nullptr) return; 

    // Run forever
    while (true) {
        // Acquire lock before checking state
        std::unique_lock<std::mutex> lock(schedulerMutex);
        
        // Go to sleep on condition variable and wait until state is RUNNING
        contextSwitchCond.wait(lock, [myself]() {
            return myself->state == TaskState::RUNNING;
        });

        // Release lock before running user function to allow preemption
        lock.unlock();

        log_task(myself->name, "STARTED");

        // Run the payload function
        if (myself->taskFunction != nullptr) {
            myself->taskFunction();
        }

        log_task(myself->name, "FINISHED");

        // Re-acquire lock to update task state safely
        lock.lock();

        // Execution finished for this period, block the task
        myself->state = TaskState::BLOCKED;
        myself->nextWakeTime += myself->period;
    }
}

bool Scheduler::AddTask(void (*func)(void), const std::string& name, uint8_t priority, uint64_t period){

    auto task = std::make_unique<TaskControlBlock>();
    task->id = ReturnAndIncreaseTaskCounter();
    task->name = name;
    task->priority = priority;
    task->dynamicPriority = priority;
    task->period = period;
    task->deadline = period;
    task->taskFunction = func;

    Scheduler::taskList.push_back(std::move(task));

    return true;
}


void Scheduler::Start(){
    std::unique_lock<std::mutex> lock(schedulerMutex); // Lock the mutex for security
    
    InitRTOSTimer();
    log_task("SCHEDULER", "Avvio del sistema e dei thread...");
    
    for (auto &task : Scheduler::taskList){
        task->osThread = std::thread(&Scheduler::TaskExecutionLoop, this);
    }

    lock.unlock();

    // Small hardware-boot-delay simulation (gives the OS time to spin up threads)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    SystemTimer::Init();
}

void Scheduler::UpdateAndSchedule(){
    std::unique_lock<std::mutex> lock(schedulerMutex);
    if (!lock.owns_lock()) {
        return; 
    }

    uint64_t currentTick = SystemTimer::GetTick();
    TaskControlBlock* nextTask = nullptr;
    uint8_t highestPriority = 255;
    
    // Update timers and check deadlines
    for (auto &task : taskList) { 
        // Check if a blocked task's period has arrived
        if ((task->state == TaskState::BLOCKED) && (currentTick >= task->nextWakeTime)) {
            task->state = TaskState::READY;
            task->deadline = currentTick + task->period;
        }
        // Hard Real-Time Constraint: Check for deadline misses
        if ((task->state == TaskState::READY || task->state == TaskState::RUNNING) && (currentTick > task->deadline)) {
            print("[ERROR] Task " + std::to_string(task->id) + " - Deadline Missed ");
            exit(100);
        }
        // Evaluate ALL ready tasks for Rate Monotonic Selection
        if ((task->state == TaskState::READY || task->state == TaskState::RUNNING) && 
            (task->dynamicPriority <= highestPriority)) {
            nextTask = task.get();
            highestPriority = task->dynamicPriority;
        }
    }
       
    // Perform Context Switch if a higher priority task is found
    if (nextTask != nullptr) {
        if (currentRunningTaskId == nextTask->id && nextTask->state == TaskState::RUNNING) {
            return;
        }

        std::string from = (currentRunningTaskId == (uint64_t)-1) ? "NONE" : std::to_string(currentRunningTaskId);
        std::string msg = "PREEMPTING: Task " + from + " -> " + std::to_string(nextTask->id);
        log_task("SCHEDULER", msg);

        // Move the previous running task back to READY state
        for (auto &task : taskList) {
            if (task->id == currentRunningTaskId && task->state == TaskState::RUNNING) {
                task->state = TaskState::READY;
                break;
            }
        }

        // Set the new task to RUNNING and update scheduler state
        nextTask->state = TaskState::RUNNING;
        currentRunningTaskId = nextTask->id;

        // Wake up the task execution threads
        contextSwitchCond.notify_all();
    } else { // IDLE
        if (currentRunningTaskId != (uint64_t)-1) {
            std::string from = std::to_string(currentRunningTaskId);
            log_task("SCHEDULER", "PREEMPTING: Task " + from + " -> IDLE");
            
            for (auto &task : taskList) {
                if (task->id == currentRunningTaskId && task->state == TaskState::RUNNING) {
                    task->state = TaskState::READY;
                    break;
                }
            }
            
            currentRunningTaskId = (uint64_t)-1; 
        }
    }
}

void Scheduler::Yield(){
    std::unique_lock<std::mutex> lock(schedulerMutex);
    
    auto myself = GetTaskByThreadId();
    if (myself->state == TaskState::RUNNING){
        lock.unlock();
        return;
    }
    if (myself->state == TaskState::READY){
        contextSwitchCond.wait(lock, [myself]() {
            return myself->state == TaskState::RUNNING;
        });
        return;
    }
    
    print("[ERROR] Task " + std::to_string(myself->id) + " in illegal state " +
            std::to_string(static_cast<int>(myself->state)));
}


