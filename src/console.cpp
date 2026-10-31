#include "console.h"
#include <iostream>
#include <string>

void Console::run(){

    std::string command;
    std::string error;

    while (true){

        std::cout << "root:\\> ";
        std::getline(std::cin, command);

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
