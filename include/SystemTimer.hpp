#ifndef _SYSTEMTIMER
#define _SYSTEMTIMER

#include <cstdint>
#include <atomic>
#include <ctime>

class SystemTimer {
private:
    static std::atomic<uint64_t> tickCount;
    static timer_t timerId;

    static void TimerISR(int sigNumber);

public:
    static void Init();
    static uint64_t GetTick();

};

#endif