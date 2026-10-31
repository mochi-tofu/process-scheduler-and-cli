
#include "config.h"
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

//to do: error checking for range limits, and checking whether rr or fcfs

static bool isPosi(const std::string& value) {
    return value.find_first_not_of("0123456789") == std::string::npos;
}

bool Config::load(const std::string& path, std::string& error){
    
    bool isValid = true;
    error = "";
    std::string line;

    //open config.txt
    std::ifstream file(path);
    if (!file.is_open()){
        error = "Failed to open " + path;
        return false;
    }

    //read per line, separate into keys and values
    while (std::getline(file, line)){
        std::istringstream iss(line);
        std::string key, value;
        iss >> key >> value;

        try {
            if (key == "num-cpu") {
                
                //check if negative
                if (!isPosi(value)) {
                    error += "Invalid value for " + key + "\n";
                    isValid = false;
                    continue;
                }

                numCpu = std::stoi(value);  //convert string to int
            }

            else if (key == "quantum-cycles"){
                
                if (!isPosi(value)) {
                    error += "Invalid value for " + key + "\n";
                    isValid = false;
                    continue;
                }

                quantumCycles = std::stoull(value);  //convert string to uint64_t
            }

            else if (key == "batch-process-freq"){
                if (!isPosi(value)) {
                    error += "Invalid value for " + key + "\n";
                    isValid = false;
                    continue;
                }
                
                batchProcessFreq = std::stoull(value);
            }

            else if (key == "min-ins"){

                if (!isPosi(value)) {
                    error += "Invalid value for " + key + "\n";
                    isValid = false;
                    continue;
                }

                minIns = std::stoull(value);
            }

            else if (key == "max-ins"){

                if (!isPosi(value)) {
                    error += "Invalid value for " + key + "\n";
                    isValid = false;
                    continue;
                }

                maxIns = std::stoull(value);
            }

            else if (key == "delay-per-exec"){

                if (!isPosi(value)) {
                    error += "Invalid value for " + key + "\n";
                    isValid = false;
                    continue;
                }

                delayPerExec = std::stoull(value);
            }

            else if (key=="scheduler"){
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {  //>=2 is for one " as input
                    scheduler = value.substr(1, value.size() - 2);
                } 
                
                else {
                    scheduler = value;
                }
            }

        } catch (const std::exception&){
            error += "Invalid value for " + key + "\n";
            isValid = false;
            continue;
        }  
        
    }

    if (numCpu < 1 || numCpu > 128) {
        error += "num-cpu must be between 1 and 128\n";
        isValid = false;
    }

    return isValid;

    std::cout << numCpu << " " << scheduler << " " << quantumCycles << " "
          << batchProcessFreq << " " << minIns << " " << maxIns << " "
          << delayPerExec << "\n";



    return true;
}