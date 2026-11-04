#pragma once
#include <memory>
#include "process.h"

class CPUCore{

private:
    int id;
    std::shared_ptr<Process> current;
    uint64_t ticksUsed = 0;

public:
    explicit CPUCore(int id);  //explicit -> safety to avoid automatic conversion in C++ when param is 1

    //getters
    std::shared_ptr<Process> getProcess() const;
    int getID() const;
    uint64_t getTicksUsed() const;

    //others
    bool isIdle() const;
    void assign (std::shared_ptr<Process> p); //assign process to core
    std::shared_ptr<Process> release(); //remove process from core
    void tick(uint64_t delayPerExec);
    
};