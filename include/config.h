#pragma once
#include <string>
#include <cstdint>

struct Config{
    int numCpu; 
    std::string scheduler;
    uint64_t quantumCycles;
    uint64_t batchProcessFreq;
    uint64_t minIns;
    uint64_t maxIns;
    uint64_t delayPerExec;

    bool load(const std::string& path, std::string& error);
};