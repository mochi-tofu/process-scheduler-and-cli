# process-scheduler-and-cli

## Group members

- Chua, Myka Nadine R.
- Lim, Julienne Skye L.
- Ong, Eiress Bassey H.
- Xu, Kai Wen H.

## Entry file

The `main` function is in `src/main.cpp`.

## Requirements

- **g++ with C++17 and thread support.**
  - Install MSYS2, then in the "MSYS2 UCRT64" terminal: `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make`
  - Add `<msys2 folder>\ucrt64\bin` to your PATH (and remove any old `C:\MinGW\bin`)
- **mingw32-make**, installed by the `mingw-w64-ucrt-x86_64-make` package above.

## Build and run

Run these from the project root (the folder with the Makefile):

| Command | What it does |
|---|---|
| `mingw32-make` | Builds the program into `main.exe` from every `src/*.cpp` |
| `./main.exe` (PowerShell) or `main.exe` (cmd) | Runs the emulator. Type `initialize` first, it reads `config.txt` |

New `.cpp` files in `src/` are picked up automatically, so the Makefile doesn't need editing when you add one.

## Building without make

If you compile by hand, use `-iquote include`, **not** `-Iinclude`:

```
g++ -std=c++17 -Wall -Wextra -iquote include src/*.cpp -o main
```

Reason: the project's `include/process.h` has the same name as a Windows system header. With `-Iinclude`,
system headers (used by threads) can accidentally load the project's file instead and fail to compile.
`-iquote` only applies to `#include "..."`, so system headers still get the real one.
