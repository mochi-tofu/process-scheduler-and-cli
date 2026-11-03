# process-scheduler-and-cli

## Requirements

- **g++ with C++17 and thread support.** We use MSYS2 UCRT64 GCC (16.2). Check with `g++ --version`.
  The old MinGW.org GCC 6.3 does **not** work (no `std::thread`, no `std::clamp`).
  - Install MSYS2, then in the "MSYS2 UCRT64" terminal: `pacman -S mingw-w64-ucrt-x86_64-gcc`
  - Add `<msys2 folder>\ucrt64\bin` to your PATH (and remove any old `C:\MinGW\bin`)
- **mingw32-make**, which comes with the MSYS2 GCC above.

## Build and run

Run these from the project root (the folder with the Makefile):

| Command | What it does |
|---|---|
| `mingw32-make` | Builds the real program into `main.exe` from every `src/*.cpp` |
| `./main.exe` (PowerShell) or `main.exe` (cmd) | Runs the emulator. Type `initialize` first, it reads `config.txt` |
| `mingw32-make test` | Builds and runs every test in `tests/` |

New `.cpp` files in `src/` are picked up automatically, so the Makefile doesn't need editing when you add one.

## Tests

- Each test is its own file `tests/<something>_test.cpp` with its own `main()`.
- Tests are linked with everything in `src/` **except** `src/main.cpp`, so they never run the console.
- To add a test, create a new `tests/<name>_test.cpp`. `mingw32-make test` finds it automatically.
- Output is one `[PASS]` / `[FAIL]` line per check.

## Building without make

If you compile by hand, use `-iquote include`, **not** `-Iinclude`:

```
g++ -std=c++17 -Wall -Wextra -iquote include src/*.cpp -o main
```

Reason: our `include/process.h` has the same name as a Windows system header. With `-Iinclude`,
system headers (used by threads) can accidentally load our file instead and fail to compile.
`-iquote` only applies to `#include "..."`, so system headers still get the real one.

## File structure

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
│   ├── Process.cpp
│   ├── ProcessGenerator.cpp
│   ├── CPUCore.cpp
│   ├── Scheduler.cpp
│   ├── Console.cpp
│   ├── Utils.cpp
│   └── main.cpp              # Entry file
├── config.txt            # Input configuration file
├── Makefile              # Optional: build with one command
└── README.txt            # Name, build/run instructions, entry file
```
