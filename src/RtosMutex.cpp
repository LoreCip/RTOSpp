#include "RtosMutex.hpp"
#include "Scheduler.hpp"
#include "utils.hpp"

extern Scheduler* globalScheduler; 

void RTOSMutex::Lock(){
    std::unique_lock<std::mutex> lock(globalScheduler->GetMutex());
    TaskControlBlock* myself = globalScheduler->GetTaskByThreadId();

    if (occupied == false){
        occupied = true;
        owner = myself; // The owner is the TCB pointer
        lock.unlock();
        return;
    }

    // Mutex is occupied, add to waiting list
    waitingList.push_back(myself);
    myself->state = TaskState::WAITING_MUTEX;

    // PRIORITY INHERITANCE
    if (owner != nullptr && myself->dynamicPriority < owner->dynamicPriority) {
        owner->dynamicPriority = myself->dynamicPriority;
    }

    globalScheduler->GetConditionVariable().wait(lock, [myself]() {
        // Wake when the currently running thread puts us on READY
        return myself->state == TaskState::READY;
    });

    owner = myself;
    myself->state = TaskState::RUNNING;
}

void RTOSMutex::Unlock(){
    std::unique_lock<std::mutex> lock(globalScheduler->GetMutex());
    TaskControlBlock* myself = globalScheduler->GetTaskByThreadId();

    if (myself != owner){
        print("[ERROR] Mismatch between lock owner and thread unlocking!");
        return;
    }

    myself->dynamicPriority = myself->priority;

    if (waitingList.empty()){
        occupied = false;
        owner = nullptr;
        return;
    }

    auto highestPriorityTaskIt = waitingList.begin();
    for (auto it = waitingList.begin(); it != waitingList.end(); ++it) {
        if ((*it)->dynamicPriority < (*highestPriorityTaskIt)->dynamicPriority) {
            highestPriorityTaskIt = it;
        }
    }

    // Estrai il task eletto dalla lista
    TaskControlBlock* nextOwner = *highestPriorityTaskIt;
    waitingList.erase(highestPriorityTaskIt);

    // Assegna direttamente il mutex al nuovo proprietario
    owner = nextOwner;
    nextOwner->state = TaskState::READY;

    // Notifica lo Schedulatore per applicare la prelazione immediata
    globalScheduler->GetConditionVariable().notify_all();


}