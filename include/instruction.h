#pragma once
#include <cstdint>
#include <string>

enum class InstructionType{ PRINT, DECLARE, ADD, SUBTRACT, SLEEP, NONE };

struct Operand {
    bool isVariable = false;
    std::string name; //already empty string upon declaration
    uint16_t value = 0;
};

struct Instruction {
    InstructionType type = InstructionType::NONE;
    std::string target;
    uint16_t value = 0;
    Operand operand1;
    Operand operand2;

};