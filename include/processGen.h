#pragma once
#include <memory>
#include <random>
#include <string>
#include <vector>
#include "config.h"
#include "process.h"

class ProcessGenerator {

    private:
        Config config;
        std::mt19937_64 rng; //random number generator, like java.util.Random

        uint64_t randBetween(uint64_t lo, uint64_t hi); //inclusive on both ends
        std::string randomVar();
        Operand randomOperand();
        Instruction randomSimple();
        std::vector<Instruction> generateBlock(uint64_t count, int depth);

    public:
        static constexpr int MAX_FOR_DEPTH = 3;   //spec: for loops can be nested up to 3 times
        static constexpr uint64_t MAX_REPEATS = 5;  //max times a FOR repeats its body
        static constexpr uint64_t MAX_BODY = 5;   //max instructions directly inside one FOR body (before expansion)

        explicit ProcessGenerator(const Config& con);  //random seed, different every run
        ProcessGenerator(const Config& con, uint64_t seed); //fixed seed, same output every run (for tests)

        //makes a process with a random instruction list, length between min-ins and max-ins (after FOR expansion)
        std::shared_ptr<Process> generate(const std::string& name, uint64_t id);
};
