#include <ctime>
#include "utils.h"

std::string getTimestamp(){
    char buffer[32];

    //current time in seconds
    std::time_t now = std::time(nullptr);

    //converts seconds into yr, month, day, hr based on local time
    std::tm* t = std::localtime(&now);

    //format output
    //strftime is a C function, so it accepts char array (which decays to a pointer to the first char)
    std::strftime(buffer, sizeof(buffer), "%m/%d/%Y %I:%M:%S%p", t);

    return std::string(buffer); //conversion can also be done implicitly
}