#include "scheduler.h"

constexpr int TICK_MS = 1; //acts like a normal variable unlike define

Scheduler::Scheduler (const Config& con): config(con){
    //create number of cores stated in config
    for (int i = 0; i < config.numCpu; i++){
        cores.push_back(CPUCore(i));
    }
}

void Scheduler::addProcess(std::shared_ptr<Process> p){
    
    std::lock_guard<std::mutex> lock(mtx);
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
        core.tick(config.delayPerExec);
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
        if (core.isIdle()){ //if no prrocess skip to next core
            continue;     
        }

        if (core.getProcess()->isFinished()){
            std::shared_ptr<Process> p = core.release();
        }

        else if (core.getProcess()->isSleeping()){
            std::shared_ptr<Process> p = core.release();
            sleepList.push_back(p);
        }

        else if (config.scheduler == "rr" && core.getTicksUsed() >= config.quantumCycles){
           std::shared_ptr<Process> p = core.release();
            p->setState(State::READY);
            readyQueue.push_back(p);
        }
    }

    cpuTicks++;
}

uint64_t Scheduler::getTicks() const { 
    std::lock_guard<std::mutex> lock(mtx);
    return cpuTicks; }

std::vector<std::shared_ptr<Process>> Scheduler::getAllProcesses() const { 
    std::lock_guard<std::mutex> lock(mtx);
    return allProcesses; }

void Scheduler::start(){
    if (running){
        return;
    }
    
    running = true;
    worker = std::thread(&Scheduler::loop, this); //1st param - points to loop function of a Scheduler
}
    
void Scheduler::loop(){

    while (running){

        {
            std::lock_guard<std::mutex> lock(mtx);
            tick();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(TICK_MS));
    }
}

void Scheduler::stop(){
    running = false;
    if (worker.joinable()) //checks if there's an existing thread for worker
        worker.join(); //console thread waits for worker thread to finish b4 continuing
}
      
//always join worker thread first b4 destroying Scheduler object
Scheduler::~Scheduler(){ stop(); }
