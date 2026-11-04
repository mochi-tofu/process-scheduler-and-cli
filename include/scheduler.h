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
#include <map>
#include <string>
#include "processGen.h"

//a copy of one process's data, so the console can print it without holding the lock
struct ProcessInfo {
    std::string name;
    uint64_t id = 0;
    std::string creationTime;
    std::vector<std::string> logs;  //only filled for process-smi
    uint64_t currentLine = 0;
    uint64_t totalLines = 0;
    int coreID = -1;
    bool finished = false;
};

//everything screen -ls / report-util print
struct SystemReport {
    int totalCores = 0;
    int coresUsed = 0;
    std::vector<ProcessInfo> running;   //currently on a core
    std::vector<ProcessInfo> finished;
};


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

        //for scheduler-start / screen -s
        ProcessGenerator generator;
        bool generating = false;     //protected by mtx (only read/written while locked), so no atomic needed
        uint64_t nextID = 1;         //shared ID counter for every created process
        uint64_t nextAutoNum = 1;    //for auto names p01, p02, ...
        std::map<std::string, std::shared_ptr<Process>> byName; //name -> process, for screen -r lookups

        //these assume mtx is ALREADY locked by the caller (std::mutex can't be locked twice by the same thread)
        void addUnlocked(std::shared_ptr<Process> p);
        void generateUnlocked();
        ProcessInfo snapshot(const std::shared_ptr<Process>& p, bool withLogs) const;

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

        //scheduler-start / scheduler-stop: turn batch generation on/off (the tick thread keeps running either way)
        void startGenerating();
        void stopGenerating();
        bool isGenerating() const;

        //screen -s: makes a random process with this name, nullptr if the name is taken
        std::shared_ptr<Process> createProcess(const std::string& name);

        //copies of process data taken while locked, safe for the console to read afterwards
        bool getProcessInfo(const std::string& name, ProcessInfo& out) const; //false if no such name
        SystemReport getReport() const;

};