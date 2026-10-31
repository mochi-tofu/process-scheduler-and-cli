#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <map>
#include "instruction.h"

enum class State{READY, RUNNING, SLEEPING, FINISHED};

class Process{

    private:
        std::string name;
        uint64_t ID;
        std::vector<Instruction> instruction_list; //list of instructions in a vector/array
        uint64_t current_line = 0;
        std::map<std::string, uint16_t> variables; //similar to hashmap
        std::vector<std::string> logs;
        State state = State::READY;
        uint16_t sleep_ticks = 0;
        std::string timestamp;
        int coreID = -1;  //no assigned core by default

    public:

        Process(const std::string& name, uint64_t id);

        //getters
        std::string getName() const;
        uint64_t getID() const;
        const std::vector<Instruction>& getInsList() const;
        uint64_t getCurrentLine() const;
        const std::map<std::string, uint16_t>& getVariables() const;
        const std::vector<std::string>& getLogs() const;
        State getState() const;
        uint16_t getSleepTicks() const;
        std::string getCreationTime() const;
        int getCoreID() const;

        //setters
        void setState(State s);
        void setCoreID(int id);

        void addInstruction(const Instruction& instruction);
        void executeNext();
        bool isFinished() const;

};