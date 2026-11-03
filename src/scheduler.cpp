#include "scheduler.h"

Scheduler::Scheduler (const Config& con): config(con){
    //create number of cores stated in config
    for (int i = 0; i < config.numCpu; i++){
        cores.push_back(CPUCore(i));
    }
}

void Scheduler::addProcess(std::shared_ptr<Process> p){
    allProcesses.push_back(p);
    readyQueue.push_back(p);
}

void Scheduler::tick(){

    //assign cores if idle
    for (CPUCore& core : cores){
        if(core.isIdle() && !readyQueue.empty()){
            core.assign(readyQueue.front());
            readyQueue.pop_front();
        }
    }

    //run each core
    for (CPUCore& core : cores){
        core.tick();
    }

    //store new updated list of sleeping processes
    //avoids messy indexing when sleeping processes in the middle of the vector wake up
    std::vector<std::shared_ptr<Process>> stillSleeping;

    for (std::shared_ptr<Process> p : sleepList){

        p->decrementSleep();

        //set state to ready when sleep ticks reach 0
        if (!p->getSleepTicks()){
            if (p->hasInsLeft()){
                p->setState(State::READY);
                readyQueue.push_back(p);
            }
            else{
                p->setState(State::FINISHED);
            }
        }

        else {
            stillSleeping.push_back(p);
        }
    }

    sleepList = stillSleeping;

    //release finished/sleeping processes from cores
    for (CPUCore& core : cores){
        if (!core.isIdle() && (core.getProcess()->isFinished() || core.getProcess()->isSleeping())){
            std::shared_ptr<Process> p = core.release();

            if (p->isSleeping())
                sleepList.push_back(p);
        }
    }

    cpuTicks++;
}

uint64_t Scheduler::getTicks() const { return cpuTicks; }
const std::vector<std::shared_ptr<Process>>& Scheduler::getAllProcesses() const { return allProcesses; }
