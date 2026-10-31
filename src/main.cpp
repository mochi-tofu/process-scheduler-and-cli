#include "console.h"
#include <iostream>

int main() {

    Console console;

    console.run();

    /*
    Config cfg;
    std::string error;
    if (!cfg.load("config.txt", error)) {
        std::cout << "Error: " << error;
        return 1;
    }
        */
    return 0;
}