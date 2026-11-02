#include "console.h"
#include <iostream>
#include <string>

void Console::run(){

    std::string command;
    std::string error;

    while (true){

        std::cout << "root:\\> ";
        
        if (!std::getline(std::cin, command)) {
            break;          // exit loop when no more input, same as "exit"
                            //if input is from file/piped or user inputs Ctrl+Z then Enter (for Windows)
        }

        //trim whitespaces in front
        command.erase(0, command.find_first_not_of(" \t\r\n")); 
        //trim whitespaces at the end
        command.erase(command.find_last_not_of(" \t\r\n") + 1);
        
        if (command.empty()) {
            continue;       // skip the rest of this iteration
        }

        if (command == "exit"){ 
            break;
        }

        else if (command == "initialize"){
            
            if(config.load("config.txt", error)){
                initialized = true;
                std::cout << "Initialization successful.\n";
            }

            else{
                std::cout << "Initialization failed.\n";
                std::cout << "Error: " << error;
            }

        }

        else if (!initialized){
            std::cout << "Run initialize first.\n";
        }

        else{
            std::cout << "Unrecognized command.\n";
        }
    }

}
