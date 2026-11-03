#pragma once
#include <memory>
#include "process.h"

class CPUCore{

private:
    int id;
    std::shared_ptr<Process> current;

public:
    explicit CPUCore(int id);  //explicit -> safety to avoid automatic conversion in C++ when param is 1

    //getters
    std::shared_ptr<Process> getProcess() const;
    int getID() const;

    //others
    bool isIdle() const;
    void assign (std::shared_ptr<Process> p); //assign process to core
    std::shared_ptr<Process> release(); //remove process from core
    void tick();
    
};