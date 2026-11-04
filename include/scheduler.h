#pragma once
#include "config.h"
#include <memory>
#include "cpuCore.h"
#include <deque>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <vector>


class Scheduler {

    private:
        Config config;
        std::vector<CPUCore> cores;
        //deque is a double ended que 
        //adding/removing elements is quick for both front and end
        std::deque<std::shared_ptr<Process>> readyQueue;
        std::vector<std::shared_ptr<Process>> allProcesses;
        uint64_t cpuTicks = 0;
        std::vector<std::shared_ptr<Process>> sleepList;
        std::thread worker; //bg thread

        //re-read from memory every time, never a cached copy  
        //like mutex but for just one variable; smth atomic cannot be interrupted midway
        //without atomic, loop() might cache running=true and never see stop() change it
        std::atomic<bool> running{false}; 

        mutable std::mutex mtx;
        void loop();

    public:
        //explicit -> safety to avoid automatic conversion in C++ when param is 1
        explicit Scheduler (const Config& con);
        void addProcess(std::shared_ptr<Process> p);
        void tick();
        uint64_t getTicks() const;
        std::vector<std::shared_ptr<Process>> getAllProcesses() const;
        void start();
        void stop();
        ~Scheduler();

};