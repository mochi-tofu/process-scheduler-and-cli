# process-scheduler-and-cli

Guide lng but feel free to change anything

```text
process scheduler/
├── include/              # Header files (Class declarations)
│   ├── Config.h              # Holds the parsed config.txt values
│   ├── Instruction.h         # PRINT, DECLARE, ADD, SUBTRACT, SLEEP (no FOR)
│   ├── Process.h             # Name, ID, instruction list, current line, variables, logs
│   ├── ProcessGenerator.h    # Creates random processes (p01, p02, ...), expands FOR
│   ├── CPUCore.h             # One core, runs one instruction per tick
│   ├── Scheduler.h           # Ready queue, FCFS/RR, running and finished lists, report
│   ├── Console.h             # Command parsing, screen command, menus
│   └── Utils.h               # Timestamp (MM/DD/YYYY HH:MM:SSAM) and string helpers
├── src/                  # Source files (Implementation logic)
│   ├── Config.cpp
│   ├── Instruction.cpp       # execute(Process&) for each instruction type
│   ├── Process.cpp
│   ├── ProcessGenerator.cpp
│   ├── CPUCore.cpp
│   ├── Scheduler.cpp
│   ├── Console.cpp
│   └── main.cpp              # Entry file
├── config.txt            # Input configuration file
├── Makefile              # Optional: build with one command
└── README.txt            # Name, build/run instructions, entry file
```
