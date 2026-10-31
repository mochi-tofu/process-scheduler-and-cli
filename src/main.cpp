#include "Config.h"
#include <iostream>

int main() {
    Config cfg;
    std::string error;
    if (!cfg.load("config.txt", error)) {
        std::cout << "Error: " << error;
        return 1;
    }
    return 0;
}