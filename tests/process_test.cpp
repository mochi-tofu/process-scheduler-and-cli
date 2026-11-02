#include "process.h"
#include <iostream>

// helpers so each test reads in one line
Instruction makeDeclare(const std::string& var, uint16_t value) {
    Instruction i;
    i.type = InstructionType::DECLARE;
    i.target = var;
    i.value = value;
    return i;
}

Operand var(const std::string& name) { Operand o; o.isVariable = true; o.name = name; return o; }
Operand num(uint16_t value)          { Operand o; o.value = value; return o; }

Instruction makeMath(InstructionType type, const std::string& target, Operand a, Operand b) {
    Instruction i;
    i.type = type;
    i.target = target;
    i.operand1 = a;
    i.operand2 = b;
    return i;
}

Instruction makeSleep(uint16_t ticks) {
    Instruction i;
    i.type = InstructionType::SLEEP;
    i.value = ticks;
    return i;
}

Instruction makePrint() {
    Instruction i;
    i.type = InstructionType::PRINT;
    return i;
}

std::string stateName(State s) {
    switch (s) {
        case State::READY:    return "READY";
        case State::RUNNING:  return "RUNNING";
        case State::SLEEPING: return "SLEEPING";
        case State::FINISHED: return "FINISHED";
    }
    return "?";
}

void check(const std::string& label, bool ok) {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << label << "\n";
}

int main() {
    // --- test 1: DECLARE, ADD, SUBTRACT, clamping, auto-declare ---
    Process p("p01", 1);
    p.addInstruction(makeDeclare("x", 10));
    p.addInstruction(makeMath(InstructionType::ADD, "y", var("x"), num(5)));            // y = 10 + 5
    p.addInstruction(makeMath(InstructionType::ADD, "z", var("undeclared"), num(7)));   // z = 0 + 7
    p.addInstruction(makeMath(InstructionType::SUBTRACT, "low", num(3), num(5)));       // 3 - 5 -> 0
    p.addInstruction(makeMath(InstructionType::ADD, "high", num(65535), num(1)));       // -> 65535
    p.addInstruction(makePrint());

    p.setCoreID(0);
    for (size_t k = 0; k < p.getInsList().size(); k++) p.executeNext();

    const auto& v = p.getVariables();
    check("DECLARE x = 10",              v.at("x") == 10);
    check("ADD y = x + 5 = 15",          v.at("y") == 15);
    check("undeclared var reads as 0",   v.at("undeclared") == 0 && v.at("z") == 7);
    check("SUBTRACT 3 - 5 clamps to 0",  v.at("low") == 0);
    check("ADD 65535 + 1 clamps",        v.at("high") == 65535);
    check("PRINT added one log",         p.getLogs().size() == 1);
    check("finished after last line",    p.isFinished());

    std::cout << "log: " << p.getLogs()[0] << "\n";

    p.executeNext();   // extra call must change nothing
    check("extra call is harmless",      p.isFinished() && p.getLogs().size() == 1);

    // --- test 2: SLEEP as the last instruction ---
    Process s("p02", 2);
    s.addInstruction(makePrint());
    s.addInstruction(makeSleep(5));

    s.executeNext();   // PRINT
    s.executeNext();   // SLEEP
    std::cout << "after SLEEP: state = " << stateName(s.getState())
              << ", sleep_ticks = " << s.getSleepTicks() << "\n";
    check("last SLEEP leaves it SLEEPING", s.getState() == State::SLEEPING);

    s.setState(State::READY);   // pretend the scheduler counted the 5 ticks down
    s.executeNext();
    check("finishes after waking up",      s.isFinished());

    // --- test 3: 0 instructions ---
    Process e("p03", 3);
    e.executeNext();
    check("empty process finishes",        e.isFinished());

    return 0;
}
