#include "console.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

//trims spaces, tabs and \r\n from both ends
static void trim(std::string& s){
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
}

void Console::printHeader() const {
    std::cout << "  ____ ____   ___  ____  _____ ______   __\n";
    std::cout << " / ___/ ___| / _ \\|  _ \\| ____/ ___\\ \\ / /\n";
    std::cout << "| |   \\___ \\| | | | |_) |  _| \\___ \\\\ V / \n";
    std::cout << "| |___ ___) | |_| |  __/| |___ ___) || |  \n";
    std::cout << " \\____|____/ \\___/|_|   |_____|____/ |_|  \n";
    std::cout << "-------------------------------------------\n";
    std::cout << "Welcome to CSOPESY Emulator!\n\n";
    std::cout << "Developers:\n";
    std::cout << "Chua, Lim, Ong, Xu\n\n";
    std::cout << "Last updated: 11-04-2026\n";
    std::cout << "-------------------------------------------\n\n";
}

void Console::clearScreen() const {
    std::system("cls");   //Windows command; on Linux/Mac it would be "clear"
}

void Console::printProcessSmi(const ProcessInfo& info) const {
    std::cout << "\nProcess name: " << info.name << "\n";
    std::cout << "ID: " << info.id << "\n";
    std::cout << "Logs:\n";
    for (const std::string& line : info.logs){
        std::cout << line << "\n";
    }
    std::cout << "\n";

    if (info.finished){
        std::cout << "Finished!\n\n";
    }
    else{
        std::cout << "Current instruction line: " << info.currentLine << "\n";
        std::cout << "Lines of code: " << info.totalLines << "\n\n";
    }
}

//the process screen after screen -s / screen -r
bool Console::screenLoop(const std::string& name){
    clearScreen();

    ProcessInfo info;
    if (scheduler->getProcessInfo(name, info)){
        printProcessSmi(info);
    }

    std::string command;
    while (true){
        std::cout << "root:\\> ";
        if (!std::getline(std::cin, command)){
            return false;
        }
        trim(command);

        if (command.empty()){
            continue;
        }

        if (command == "exit"){
            clearScreen();
            printHeader();
            return true;
        }

        else if (command == "process-smi"){
            //fresh copy every time, the process keeps running in the background
            if (scheduler->getProcessInfo(name, info)){
                printProcessSmi(info);
            }
        }

        else{
            std::cout << "Unrecognized command.\n";
        }
    }
}

std::string Console::buildReport() const {
    SystemReport r = scheduler->getReport();
    std::ostringstream out;   //like a cout that writes into a string instead of the screen

    int utilization = r.totalCores ? (r.coresUsed * 100 / r.totalCores) : 0;
    out << "CPU utilization: " << utilization << "%\n";
    out << "Cores used: " << r.coresUsed << "\n";
    out << "Cores available: " << (r.totalCores - r.coresUsed) << "\n\n";
    out << "--------------------------------------\n";

    //std::left + std::setw(n): pad the next value with spaces to n characters, so columns line up
    out << "Running processes:\n";
    for (const ProcessInfo& p : r.running){
        out << std::left << std::setw(12) << p.name
            << "(" << p.creationTime << ")   "
            << "Core: " << std::setw(5) << p.coreID
            << p.currentLine << " / " << p.totalLines << "\n";
    }

    out << "\nFinished processes:\n";
    for (const ProcessInfo& p : r.finished){
        out << std::left << std::setw(12) << p.name
            << "(" << p.creationTime << ")   "
            << std::setw(11) << "Finished"
            << p.currentLine << " / " << p.totalLines << "\n";
    }
    out << "--------------------------------------\n";

    return out.str();
}

void Console::run(){

    std::string command;
    std::string error;

    printHeader();

    while (true){

        std::cout << "root:\\> ";

        if (!std::getline(std::cin, command)) {
            break;          // exit loop when no more input, same as "exit"
                            //if input is from file/piped or user inputs Ctrl+Z then Enter (for Windows)
        }

        trim(command);

        if (command.empty()) {
            continue;       // skip the rest of this iteration
        }

        //split into words: "screen -s p01" -> cmd="screen", flag="-s", name="p01"
        std::istringstream words(command);
        std::string cmd, flag, name;
        words >> cmd >> flag >> name;

        if (command == "exit"){
            break;
        }

        else if (command == "initialize"){

            if (initialized){
                std::cout << "Already initialized.\n";   //config can't change while the scheduler is running
            }

            else if(config.load("config.txt", error)){
                initialized = true;
                scheduler = std::make_unique<Scheduler>(config);
                scheduler->start();   //spec: the scheduler runs as long as the console is alive
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

        else if (cmd == "screen" && flag == "-s" && !name.empty()){
            if (scheduler->createProcess(name)){
                if (!screenLoop(name)) break;
            }
            else{
                std::cout << "Process " << name << " already exists.\n";
            }
        }

        else if (cmd == "screen" && flag == "-r" && !name.empty()){
            ProcessInfo info;
            //spec: not found if it doesn't exist OR already finished
            if (scheduler->getProcessInfo(name, info) && !info.finished){
                if (!screenLoop(name)) break;
            }
            else{
                std::cout << "Process " << name << " not found.\n";
            }
        }

        else if (command == "screen -ls"){
            std::cout << buildReport() << "\n";
        }

        else if (command == "report-util"){
            std::ofstream file("csopesy-log.txt");
            file << buildReport();
            std::cout << "Report generated at " << std::filesystem::absolute("csopesy-log.txt").string() << "!\n";
        }

        else if (command == "scheduler-start"){
            if (scheduler->isGenerating()){
                std::cout << "Scheduler is already generating processes.\n";
            }
            else{
                scheduler->startGenerating();
                std::cout << "Scheduler started generating processes.\n";
            }
        }

        else if (command == "scheduler-stop"){
            if (!scheduler->isGenerating()){
                std::cout << "Scheduler is not generating processes.\n";
            }
            else{
                scheduler->stopGenerating();
                std::cout << "Scheduler stopped generating processes.\n";
            }
        }

        else{
            std::cout << "Unrecognized command.\n";
        }
    }

}
