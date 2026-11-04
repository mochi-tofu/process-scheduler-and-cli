# Claude generated codes

Code in this project that was written by Claude (AI assistant) directly, rather than drafted by a group member first.

## Files written completely by Claude

| File | What it is |
|---|---|
| [tests/process_test.cpp](../tests/process_test.cpp) | 11 checks for `Process::executeNext` |
| [tests/scheduler_test.cpp](../tests/scheduler_test.cpp) | FCFS, SLEEP, RR and delay-per-exec tests (49 checks) |
| [tests/thread_test.cpp](../tests/thread_test.cpp) | Background thread tests: start, stop, destructor, locking (10 checks) |
| [tests/generator_test.cpp](../tests/generator_test.cpp) | ProcessGenerator and scheduler-start tests (25 checks) |
| [include/processGen.h](../include/processGen.h), [src/processGen.cpp](../src/processGen.cpp) | ProcessGenerator: random instructions, FOR loops expanded into a flat list, length within min-ins and max-ins |

## Parts of existing files changed by Claude

### [src/scheduler.cpp](../src/scheduler.cpp) and [include/scheduler.h](../include/scheduler.h)

- **Constructor:** added `generator(con)` to the initializer list.
- **`addProcess`:** the lock stays, but the body moved into a new `addUnlocked()`, which also records the name in `byName`.
- **End of `tick()`:** added the generation hook, `if (generating && cpuTicks % batchProcessFreq == 0) generateUnlocked();`
- **`loop()`:** rewritten with catch-up logic (runs the number of ticks owed by real elapsed time, because a 1ms sleep on Windows actually lasts about 16ms). The original lock-around-tick and sleep-outside-lock structure is kept inside it.
- **New members in the header:** `generator`, `generating`, `nextID`, `nextAutoNum`, `byName`, and the `ProcessInfo` and `SystemReport` structs.
- **New functions:** `generateUnlocked`, `startGenerating`, `stopGenerating`, `isGenerating`, `createProcess`, `snapshot`, `getProcessInfo`, `getReport`.

Not changed by Claude: `tick()` itself (apart from the hook at the end), `start`, `stop`, the destructor, `getTicks`, `getAllProcesses`.

### [src/console.cpp](../src/console.cpp) and [include/console.h](../include/console.h)

Kept from the original `run()` loop: the prompt, getline/EOF handling, trimming, `exit`, `initialize`, "Run initialize first." and "Unrecognized command."

Changed or added by Claude:
- The trim lines moved into a `trim()` helper, so the screen loop can reuse them.
- `initialize` now refuses a second run, creates the scheduler and starts its thread.
- New: the header banner, `screen -s`, `screen -r`, `screen -ls`, `scheduler-start`, `scheduler-stop`, `report-util`, and the helpers `screenLoop`, `printProcessSmi`, `buildReport`, plus the `std::unique_ptr<Scheduler>` member.
