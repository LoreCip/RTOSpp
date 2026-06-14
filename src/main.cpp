#include <iostream>
#include <chrono>
#include <thread>
#include "Scheduler.hpp"
#include "SystemTimer.hpp"
#include "RtosMutex.hpp"
#include "utils.hpp"

Scheduler* globalScheduler = nullptr;
RTOSMutex navMutex;

struct Coordinate {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};
Coordinate globalCoord;

// ============================================================================
// TASK 1: CONTROLLO MOTORI (Priorità: 0, Periodo: 10ms)
// ============================================================================
void Task_FlightControl_High() {
    Coordinate localCopy;

    // Sezione critica rapidissima
    navMutex.Lock();
    localCopy = globalCoord; 
    navMutex.Unlock();

    // Stampa con il nuovo log preciso senza passare il Tick manualmente
    log_task("Flight_Ctrl", "Motori aggiornati su X: " + std::to_string(localCopy.x));
}

// ============================================================================
// TASK 2: TELEMETRIA (Priorità: 5, Periodo: 30ms)
// ============================================================================
void Task_Telemetry_Medium() {
    log_task("Telemetry", "STARTED");
    log_task("TELEMETRY", "Invio radio in corso...");
    
    for (int i = 0; i < 200000; i++) {
        double finto_calcolo = i * 3.14; (void)finto_calcolo;
        globalScheduler->GetTaskByThreadId(); // Yield implicito per il simulatore
    }
    
    log_task("Telemetry", "FINISHED");
}

// ============================================================================
// TASK 3: CALCOLO ROTTA GPS (Priorità: 10, Periodo: 50ms)
// ============================================================================
void Task_Navigation_Low() {
    log_task("Navigation", "STARTED");
    log_task("NAVIGATION", "Calcolo nuova rotta satellitare...");

    double new_x = 0, new_y = 0, new_z = 0;
    for (int i = 0; i < 400000; i++) {
        new_x += 0.001; 
        new_y += 0.002;
        globalScheduler->GetTaskByThreadId(); // Yield implicito per il simulatore
    }

    // Scrittura rapida
    navMutex.Lock();
    globalCoord.x = new_x;
    globalCoord.y = new_y;
    globalCoord.z = new_z;
    navMutex.Unlock();

    log_task("NAVIGATION", "Coordinate salvate nel sistema globale.");
    log_task("Navigation", "FINISHED");
}

int main() {
    Scheduler rtosScheduler;
    globalScheduler = &rtosScheduler;

    rtosScheduler.AddTask(Task_FlightControl_High, "Flight_Ctrl", 0, 10);
    rtosScheduler.AddTask(Task_Telemetry_Medium,   "Telemetry",   5, 30);
    rtosScheduler.AddTask(Task_Navigation_Low,     "Navigation",  10, 50);

    rtosScheduler.Start();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}