#ifndef _UTILS
#define _UTILS

#include <chrono>
#include <string>
#include <cstdio>
#include <iostream>

inline void print(std::string s){
    std::cout << s << std::endl;
}

// Variabile globale da inizializzare quando chiami Scheduler::Start()
extern std::chrono::time_point<std::chrono::high_resolution_clock> rtosStartTime;

inline void InitRTOSTimer() {
    rtosStartTime = std::chrono::high_resolution_clock::now();
}

// Nuova funzione per ottenere il tempo decimale preciso
inline double GetPreciseTime() {
    auto now = std::chrono::high_resolution_clock::now();
    // Calcoliamo la differenza in microsecondi per massima precisione
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - rtosStartTime);
    // Convertiamo in millisecondi decimali (es. 10.234 ms)
    return duration.count() / 1000.0;
}

inline void log_task(const std::string& taskName, const std::string& message) {
    std::printf("[%.3f] [%s] %s\n", GetPreciseTime(), taskName.c_str(), message.c_str());
}

#endif