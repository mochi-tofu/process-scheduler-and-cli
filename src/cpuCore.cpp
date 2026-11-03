#include "cpuCore.h"

CPUCore::CPUCore(int id) : id(id) {}

//getters
std::shared_ptr<Process> CPUCore::getProcess() const{
    return current;
}

int CPUCore::getID() const{ return id;}

//others
bool CPUCore::isIdle() const{
    return !current;
}

//assign process to core
void CPUCore::assign (std::shared_ptr<Process> p){
    current = p;
    p->setCoreID(id);
    p->setState(State::RUNNING);
} 

//remove process from core
std::shared_ptr<Process> CPUCore::release(){
    std::shared_ptr<Process> temp = current;
    current = nullptr;

    //in case current was empty from the start
    if (temp)
        temp->setCoreID(-1);

    return temp;
} 

void CPUCore::tick(){
    if (current){
        current->executeNext();
    }
}
    