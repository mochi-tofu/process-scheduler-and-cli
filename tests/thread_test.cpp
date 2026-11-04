#include "scheduler.h"
#include <iostream>

// tests the background thread: start(), stop(), destructor, and the locked getters

std::shared_ptr<Process> makeProcess(const std::string& name, uint64_t id, int numPrints) {
    auto p = std::make_shared<Process>(name, id);
    for (int k = 0; k < numPrints; k++) {
        Instruction i;
        i.type = InstructionType::PRINT;
        p->addInstruction(i);
    }
    return p;
}

Config makeConfig(int numCpu, const std::string& scheduler, uint64_t quantum = 1, uint64_t delay = 0) {
    Config c;
    c.numCpu = numCpu;
    c.scheduler = scheduler;
    c.quantumCycles = quantum;
    c.delayPerExec = delay;
    return c;
}

// waits (in real time) until the worker has done at least `ticks` ticks, or gives up after 5s
// only uses getTicks(), which locks, so it's safe to call while the worker runs
bool waitForTicks(const Scheduler& s, uint64_t ticks) {
    for (int ms = 0; ms < 5000; ms++) {
        if (s.getTicks() >= ticks) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}

void check(const std::string& label, bool ok) {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << label << "\n";
}

int main() {
    // --- test 1: nothing ticks before start() ---
    {
        Scheduler s(makeConfig(1, "fcfs"));
        s.addProcess(makeProcess("p01", 1, 3));
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        check("before start: no ticks", s.getTicks() == 0);
    }

    // --- test 2: start() makes ticks happen on their own, processes finish ---
    {
        Scheduler s(makeConfig(2, "rr", 2));
        auto p1 = makeProcess("p01", 1, 5);
        auto p2 = makeProcess("p02", 2, 5);
        auto p3 = makeProcess("p03", 3, 5);
        s.addProcess(p1);
        s.addProcess(p2);
        s.addProcess(p3);

        s.start();
        check("after start: ticks advance by themselves", waitForTicks(s, 20));
        s.stop();

        // read process state only after stop(): the worker is gone, so no lock needed
        check("after stop: all 3 processes finished",
              p1->isFinished() && p2->isFinished() && p3->isFinished());
    }

    // --- test 3: stop() really stops the ticking ---
    {
        Scheduler s(makeConfig(1, "fcfs"));
        s.start();
        waitForTicks(s, 5);
        s.stop();
        uint64_t afterStop = s.getTicks();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        check("after stop: ticks no longer change", s.getTicks() == afterStop);
    }

    // --- test 4: calling start() twice doesn't make a second thread (would crash: thread already running) ---
    {
        Scheduler s(makeConfig(1, "fcfs"));
        s.start();
        s.start();
        s.stop();
        check("start twice: no crash", true);
    }

    // --- test 5: stop() without start(), and stop() twice, are both harmless ---
    {
        Scheduler s(makeConfig(1, "fcfs"));
        s.stop();
        s.start();
        s.stop();
        s.stop();
        check("stop without start / stop twice: no crash", true);
    }

    // --- test 6: start, stop, start again works (scheduler-stop then scheduler-start) ---
    {
        Scheduler s(makeConfig(1, "fcfs"));
        s.start();
        waitForTicks(s, 5);
        s.stop();
        uint64_t first = s.getTicks();
        s.start();
        check("restart: ticks continue from where they stopped", waitForTicks(s, first + 5));
        s.stop();
    }

    // --- test 7: destructor joins the thread (no stop() call; std::terminate would crash here otherwise) ---
    {
        Scheduler s(makeConfig(1, "fcfs"));
        s.start();
        waitForTicks(s, 3);
    }   // s destroyed here while running
    check("destructor: running scheduler destroyed without crash", true);

    // --- test 8: adding processes from this thread while the worker runs (the lock in addProcess) ---
    {
        Scheduler s(makeConfig(4, "rr", 3));
        s.start();
        std::vector<std::shared_ptr<Process>> added;
        for (int k = 1; k <= 200; k++) {
            auto p = makeProcess("p" + std::to_string(k), k, 10);
            added.push_back(p);
            s.addProcess(p);
            s.getAllProcesses();   // also hammer the locked getter at the same time
        }
        // 200 x 10 instructions over 4 cores needs ~500 ticks, wait for plenty
        waitForTicks(s, 1000);
        s.stop();

        bool allDone = true;
        for (const auto& p : added) {
            if (!p->isFinished()) allDone = false;
        }
        check("add while running: all 200 recorded", s.getAllProcesses().size() == 200);
        check("add while running: all 200 finished", allDone);
    }

    return 0;
}
