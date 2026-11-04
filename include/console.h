#pragma once
#include <memory>
#include <string>
#include "config.h"
#include "scheduler.h"

class Console {
    public:
        void run();
    private:
        Config config;
        bool initialized = false;

        //unique_ptr: Scheduler can't be copied (it owns a thread and a mutex), and it only exists after initialize
        std::unique_ptr<Scheduler> scheduler;

        void printHeader() const;
        void clearScreen() const;
        bool screenLoop(const std::string& name);   //false if input ended (Ctrl+Z), so run() can stop too
        void printProcessSmi(const ProcessInfo& info) const;
        std::string buildReport() const;            //shared by screen -ls and report-util
};
