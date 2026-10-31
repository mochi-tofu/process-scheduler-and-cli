#pragma once
#include "config.h"

class Console {
    public:
        void run();
    private:
        Config config;
        bool initialized = false;
};