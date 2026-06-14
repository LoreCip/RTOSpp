#include "SystemTimer.hpp"
#include "Scheduler.hpp"
#include <csignal>
#include <iostream>

#include "utils.hpp"
#include "constants.hpp"

extern Scheduler* globalScheduler; // Definito nel main
std::atomic<uint64_t> SystemTimer::tickCount{0};
timer_t SystemTimer::timerId;

void SystemTimer::Init() {
    struct sigaction sa = {};                    // data structure in POSIX standard <csignal>
    sa.sa_handler = &SystemTimer::TimerISR;      // contains the address of the function to run when signal arrives
    sa.sa_flags = SA_RESTART;                    // continue running the timer if interrupted by the OS for other system calls
    sigemptyset(&sa.sa_mask);

    sigaction(SIGRTMIN, &sa, nullptr);            // run the function when SIGRTMIN is recieved

    struct sigevent se = {};                     // define the event
    se.sigev_notify = SIGEV_SIGNAL;              // when event happens, send a signal
    se.sigev_signo = SIGRTMIN;                    // which signal to send?
    
    timer_create(CLOCK_MONOTONIC, &se, &SystemTimer::timerId); 

    struct itimerspec its = {};                  // define the intervals
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = FREQUENCY; 
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = FREQUENCY;

    timer_settime(SystemTimer::timerId, 0, &its, nullptr);
}

uint64_t SystemTimer::GetTick(){
    return tickCount.load();
}

void SystemTimer::TimerISR(int sigNumber) {
    if (sigNumber == SIGRTMIN) {
        int overrun = timer_getoverrun(timerId);
        uint64_t ticksToAdd = 1 + (overrun >= 0 ? overrun : 0);
        SystemTimer::tickCount.fetch_add(ticksToAdd, std::memory_order_relaxed);

        if (globalScheduler != nullptr) {
            globalScheduler->UpdateAndSchedule();
        }

    } else {
        print("[ERROR] sigNumber != SIGRTMIN in system timer");
    }

}