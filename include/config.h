#pragma once  //make sure only included once
#include <string>
#include <cstdint>

struct Config{
    int numCpu = 0; 
    std::string scheduler = "";
    uint64_t quantumCycles = 0;
    uint64_t batchProcessFreq = 0;
    uint64_t minIns = 0;
    uint64_t maxIns = 0;
    uint64_t delayPerExec = UINT64_MAX;

    bool load(const std::string& path, std::string& error);
};