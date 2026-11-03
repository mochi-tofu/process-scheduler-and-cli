#include "process.h"
#include "utils.h"
#include <iostream>
#include <algorithm>

Process::Process(const std::string& name, uint64_t id)
    : name(name), ID(id){
        timestamp = getTimeNow();
    }

//getters
std::string Process::getName() const{ return name; }
uint64_t Process::getID() const { return ID; }
const std::vector<Instruction>& Process::getInsList() const { return instruction_list; }
uint64_t Process::getCurrentLine() const { return current_line; }
const std::map<std::string, uint16_t>& Process::getVariables() const { return variables; }
const std::vector<std::string>& Process::getLogs() const { return logs; }
State Process::getState() const { return state; }
uint16_t Process::getSleepTicks() const { return sleep_ticks; }
std::string Process::getCreationTime() const { return timestamp; }
int Process::getCoreID() const { return coreID; }

//setters
void Process::setState(State s){ state = s; }
void Process::setCoreID(int id) { coreID = id; }

//others
void Process::addInstruction(const Instruction& instruction) { instruction_list.push_back(instruction); }
bool Process::isFinished() const{ return state == State::FINISHED; }
bool Process::isSleeping() const{ return state == State::SLEEPING; }

//set operand to be given value or retrieve from variable list
uint16_t Process::resolve(const Operand& op){

    if (op.isVariable)
        return variables[op.name];

    else
        return op.value;

}

void Process::executeNext(){

    if (current_line >= instruction_list.size()){
        state = State::FINISHED; //for processes with 0 instructions, or just woke up form SLEEPING
        return;
    }

    const Instruction& ins = instruction_list[current_line];

    switch(ins.type){

        case InstructionType::PRINT:{
            std::string line = "(" + getTimeNow() + ") Core:" + std::to_string(coreID) + " \"Hello world from " + name + "!\"";
            logs.push_back(line);             // push_back works like ArrayList.add

            break;
        }

        case InstructionType::DECLARE:
            variables[ins.target] = ins.value;
            break;

        case InstructionType::ADD:
            variables[ins.target] = std::clamp(resolve(ins.operand1) + resolve(ins.operand2), 0, 65535);
            break;

        case InstructionType::SUBTRACT:
            variables[ins.target] = std::clamp(resolve(ins.operand1) - resolve(ins.operand2), 0, 65535);
            break;

        case InstructionType::SLEEP:{
            sleep_ticks = ins.value;

            if (ins.value)  //only changes state if sleep ticks value is above 0
                state = State::SLEEPING;
            break;
        }

        case InstructionType::NONE: break;
    }

    //next instruction
    current_line++;
    
    if (current_line >= instruction_list.size() && state != State::SLEEPING){
        state = State::FINISHED;
    }

}

void Process::decrementSleep(){
    if (sleep_ticks > 0) //sleep_ticks is uint16_t, so can't be negative
        sleep_ticks--;
}

bool Process::hasInsLeft() const{
    return current_line < instruction_list.size();
}
