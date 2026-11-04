#include "scheduler.h"

constexpr int TICK_MS = 1; //acts like a normal variable unlike define

Scheduler::Scheduler (const Config& con): config(con), generator(con){
    //create number of cores stated in config
    for (int i = 0; i < config.numCpu; i++){
        cores.push_back(CPUCore(i));
    }
}

void Scheduler::addProcess(std::shared_ptr<Process> p){
    
    std::lock_guard<std::mutex> lock(mtx);
    addUnlocked(p);
}

//the actual adding, without locking
//used by addProcess (after it locks) and by tick() (already locked by loop())
void Scheduler::addUnlocked(std::shared_ptr<Process> p){
    allProcesses.push_back(p);
    readyQueue.push_back(p);
    byName[p->getName()] = p;
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

    //scheduler-start: a new process every batch-process-freq ticks, at the END of the tick
    //(spec: "if one, a new process is generated at the end of each CPU cycle")
    if (generating && cpuTicks % config.batchProcessFreq == 0){
        generateUnlocked();
    }
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

    //windows only wakes sleeping threads every ~16ms, so "tick then sleep 1ms" really gives ~62 ticks/sec
    //instead: check the real clock each time we wake up, and run however many ticks should have happened by now
    //(steady_clock = stopwatch clock, only moves forward)
    auto start = std::chrono::steady_clock::now();
    uint64_t done = 0; //ticks run since this loop() started (local, so start()/stop()/start() begins again from 0)

    while (running){

        //milliseconds since start: (now - start) is a duration, cast it to ms, .count() makes it a plain number
        uint64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - start).count();
        uint64_t target = elapsed / TICK_MS; //ticks that should have happened by now

        //catch up, e.g. after a ~16ms sleep this runs ~16 ticks
        while (done < target && running){
            {
                std::lock_guard<std::mutex> lock(mtx); //lock per tick, so the console can get in between catch-up ticks
                tick();
            }
            done++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1)); //outside the lock; how long it really sleeps doesn't matter anymore
    }
}

void Scheduler::stop(){
    running = false;
    if (worker.joinable()) //checks if there's an existing thread for worker
        worker.join(); //console thread waits for worker thread to finish b4 continuing
}
      
//always join worker thread first b4 destroying Scheduler object
Scheduler::~Scheduler(){ stop(); }


//makes the next auto-named process (p01, p02, ...) and adds it
//skips a name if the user already took it with screen -s
void Scheduler::generateUnlocked(){
    std::string name;
    do {
        std::string num = std::to_string(nextAutoNum++);
        if (num.size() < 2) num = "0" + num;  //p01..p09, then p10, p100, p1240
        name = "p" + num;
    } while (byName.count(name));

    addUnlocked(generator.generate(name, nextID++));
}

void Scheduler::startGenerating(){
    std::lock_guard<std::mutex> lock(mtx);
    generating = true;
}

void Scheduler::stopGenerating(){
    std::lock_guard<std::mutex> lock(mtx);
    generating = false;
}

bool Scheduler::isGenerating() const {
    std::lock_guard<std::mutex> lock(mtx);
    return generating;
}

std::shared_ptr<Process> Scheduler::createProcess(const std::string& name){
    std::lock_guard<std::mutex> lock(mtx);

    if (byName.count(name)){ //names must be unique so screen -r knows which one you mean
        return nullptr;
    }

    std::shared_ptr<Process> p = generator.generate(name, nextID++);
    addUnlocked(p);
    return p;
}

//copies the fields the console needs; caller must hold mtx
ProcessInfo Scheduler::snapshot(const std::shared_ptr<Process>& p, bool withLogs) const {
    ProcessInfo info;
    info.name = p->getName();
    info.id = p->getID();
    info.creationTime = p->getCreationTime();
    if (withLogs) info.logs = p->getLogs();  //copies the vector
    info.currentLine = p->getCurrentLine();
    info.totalLines = p->getInsList().size();
    info.coreID = p->getCoreID();
    info.finished = p->isFinished();
    return info;
}

bool Scheduler::getProcessInfo(const std::string& name, ProcessInfo& out) const {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = byName.find(name);
    if (it == byName.end()){
        return false;
    }

    out = snapshot(it->second, true);
    return true;
}

SystemReport Scheduler::getReport() const {
    std::lock_guard<std::mutex> lock(mtx);

    SystemReport r;
    r.totalCores = cores.size();

    //running = whatever is on a core right now, in core order
    for (const CPUCore& core : cores){
        if (!core.isIdle()){
            r.coresUsed++;
            r.running.push_back(snapshot(core.getProcess(), false));
        }
    }

    //finished, in the order they were created
    for (const std::shared_ptr<Process>& p : allProcesses){
        if (p->isFinished()){
            r.finished.push_back(snapshot(p, false));
        }
    }

    return r;
}
