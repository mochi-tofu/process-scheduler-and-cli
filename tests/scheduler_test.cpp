#include "scheduler.h"
#include <iostream>

// helpers so each test reads in one line
Instruction makePrint() {
    Instruction i;
    i.type = InstructionType::PRINT;
    return i;
}

std::shared_ptr<Process> makeProcess(const std::string& name, uint64_t id, int numPrints) {
    auto p = std::make_shared<Process>(name, id);
    for (int k = 0; k < numPrints; k++) p->addInstruction(makePrint());
    return p;
}

Instruction makeSleep(uint16_t ticks) {
    Instruction i;
    i.type = InstructionType::SLEEP;
    i.value = ticks;
    return i;
}

// builds a process from a list of instructions, e.g. { makePrint(), makeSleep(2) }
std::shared_ptr<Process> makeProcess(const std::string& name, uint64_t id,
                                     const std::vector<Instruction>& instructions) {
    auto p = std::make_shared<Process>(name, id);
    for (const auto& ins : instructions) p->addInstruction(ins);
    return p;
}

Config makeConfig(int numCpu, const std::string& scheduler, uint64_t quantum = 1, uint64_t delay = 0) {
    Config c;
    c.numCpu = numCpu;
    c.scheduler = scheduler;
    c.quantumCycles = quantum;
    c.delayPerExec = delay;   // Config's default is UINT64_MAX (means "missing"), so always set it in tests
    return c;
}

bool allFinished(const Scheduler& s) {
    for (const auto& p : s.getAllProcesses()) {
        if (!p->isFinished()) return false;
    }
    return true;
}

// keeps ticking until every process is done (safety cap so a bug can't loop forever)
void runUntilDone(Scheduler& s, uint64_t maxTicks = 100) {
    while (!allFinished(s) && s.getTicks() < maxTicks) s.tick();
}

// true if every log line of p says the given core
bool ranOnlyOn(const std::shared_ptr<Process>& p, int core) {
    std::string tag = "Core:" + std::to_string(core) + " ";
    for (const auto& line : p->getLogs()) {
        if (line.find(tag) == std::string::npos) return false;
    }
    return true;
}

void check(const std::string& label, bool ok) {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << label << "\n";
}

int main() {
    // --- test 1: FCFS, 1 core, 3 processes x 3 PRINTs ---
    {
        Scheduler s(makeConfig(1, "fcfs"));
        auto p1 = makeProcess("p01", 1, 3);
        auto p2 = makeProcess("p02", 2, 3);
        auto p3 = makeProcess("p03", 3, 3);
        s.addProcess(p1);
        s.addProcess(p2);
        s.addProcess(p3);

        // after 3 ticks only p01 should be done, p02 not started yet
        for (int k = 0; k < 3; k++) s.tick();
        check("1 core: p01 finished after 3 ticks",  p1->isFinished());
        check("1 core: p02 not started yet",         p2->getCurrentLine() == 0);

        runUntilDone(s);
        check("1 core: all finished",                allFinished(s));
        check("1 core: took exactly 9 ticks",        s.getTicks() == 9);
        check("1 core: everything ran on Core:0",
              ranOnlyOn(p1, 0) && ranOnlyOn(p2, 0) && ranOnlyOn(p3, 0));
        check("1 core: each process logged 3 times",
              p1->getLogs().size() == 3 && p2->getLogs().size() == 3 && p3->getLogs().size() == 3);
        check("1 core: released processes have coreID -1",
              p1->getCoreID() == -1 && p2->getCoreID() == -1 && p3->getCoreID() == -1);
    }

    // --- test 2: FCFS, 2 cores ---
    {
        Scheduler s(makeConfig(2, "fcfs"));
        auto p1 = makeProcess("p01", 1, 3);
        auto p2 = makeProcess("p02", 2, 3);
        auto p3 = makeProcess("p03", 3, 3);
        s.addProcess(p1);
        s.addProcess(p2);
        s.addProcess(p3);

        s.tick();
        check("2 cores: p01 and p02 start together", p1->getCurrentLine() == 1 && p2->getCurrentLine() == 1);
        check("2 cores: p03 waits",                  p3->getCurrentLine() == 0);

        runUntilDone(s);
        check("2 cores: took exactly 6 ticks",       s.getTicks() == 6);
        check("2 cores: p01 on Core:0, p02 on Core:1", ranOnlyOn(p1, 0) && ranOnlyOn(p2, 1));
        check("2 cores: p03 reused the first free core", ranOnlyOn(p3, 0));
    }

    // --- test 3: more cores than processes ---
    {
        Scheduler s(makeConfig(4, "fcfs"));
        auto p1 = makeProcess("p01", 1, 2);
        s.addProcess(p1);
        runUntilDone(s);
        check("4 cores, 1 process: finishes in 2 ticks", s.getTicks() == 2 && p1->isFinished());
    }

    // --- test 4: ticking with nothing to run is harmless ---
    {
        Scheduler s(makeConfig(2, "fcfs"));
        for (int k = 0; k < 5; k++) s.tick();
        check("empty scheduler: ticks still count",  s.getTicks() == 5);
    }

    // --- test 5: SLEEP frees the core for another process ---
    {
        Scheduler s(makeConfig(1, "fcfs"));
        auto p1 = makeProcess("p01", 1, { makePrint(), makeSleep(2), makePrint() });
        auto p2 = makeProcess("p02", 2, 3);
        s.addProcess(p1);
        s.addProcess(p2);

        s.tick();   // tick 0: p01 PRINT
        s.tick();   // tick 1: p01 SLEEP(2) -> leaves the core
        check("sleep: p01 is SLEEPING after its SLEEP",     p1->isSleeping());
        check("sleep: p01 is off the core",                 p1->getCoreID() == -1);

        s.tick();   // tick 2: p02 takes the core
        check("sleep: p02 runs while p01 sleeps",           p2->getCurrentLine() == 1);

        runUntilDone(s);
        check("sleep: took exactly 6 ticks",                s.getTicks() == 6);
        check("sleep: p01 printed twice, p02 three times",
              p1->getLogs().size() == 2 && p2->getLogs().size() == 3);
    }

    // --- test 6: SLEEP as the last instruction finishes as soon as the sleep ends ---
    {
        Scheduler s(makeConfig(1, "fcfs"));
        auto p1 = makeProcess("p01", 1, { makePrint(), makeSleep(2) });
        s.addProcess(p1);
        runUntilDone(s);
        check("sleep last: finished",                       p1->isFinished());
        check("sleep last: took exactly 4 ticks",           s.getTicks() == 4);
    }

    // --- test 7: two processes sleeping at the same time, waking in a different order ---
    {
        Scheduler s(makeConfig(2, "fcfs"));
        auto p1 = makeProcess("p01", 1, { makeSleep(3), makePrint() });
        auto p2 = makeProcess("p02", 2, { makeSleep(1), makePrint() });
        s.addProcess(p1);
        s.addProcess(p2);

        s.tick();   // tick 0: both go to sleep
        check("two sleepers: both SLEEPING",                p1->isSleeping() && p2->isSleeping());

        runUntilDone(s);
        check("two sleepers: both finished",                p1->isFinished() && p2->isFinished());
        check("two sleepers: took exactly 5 ticks",         s.getTicks() == 5);
        check("two sleepers: each printed once",
              p1->getLogs().size() == 1 && p2->getLogs().size() == 1);
    }

    // --- test 8: SLEEP(0) does nothing ---
    {
        Scheduler s(makeConfig(1, "fcfs"));
        auto p1 = makeProcess("p01", 1, { makeSleep(0), makePrint() });
        s.addProcess(p1);
        s.tick();
        check("sleep(0): never goes to SLEEPING",           !p1->isSleeping() && p1->getCoreID() == 0);
        runUntilDone(s);
        check("sleep(0): took exactly 2 ticks",             s.getTicks() == 2);
    }

    // --- test 9: RR, 1 core, quantum 2 -> processes take turns ---
    {
        Scheduler s(makeConfig(1, "rr", 2));
        auto p1 = makeProcess("p01", 1, 4);
        auto p2 = makeProcess("p02", 2, 4);
        s.addProcess(p1);
        s.addProcess(p2);

        s.tick(); s.tick();   // ticks 0-1: p01's turn
        check("rr: p01 ran 2 then was preempted",           p1->getCurrentLine() == 2 && p1->getCoreID() == -1);
        check("rr: preempted p01 is READY",                 p1->getState() == State::READY);

        s.tick(); s.tick();   // ticks 2-3: p02's turn
        check("rr: p02 got its turn",                       p2->getCurrentLine() == 2);
        check("rr: p01 did not run during p02's turn",      p1->getCurrentLine() == 2);

        runUntilDone(s);
        check("rr: took exactly 8 ticks",                   s.getTicks() == 8);
        check("rr: each printed 4 times",
              p1->getLogs().size() == 4 && p2->getLogs().size() == 4);
    }

    // --- test 10: same input with FCFS does NOT take turns ---
    {
        Scheduler s(makeConfig(1, "fcfs", 2));
        auto p1 = makeProcess("p01", 1, 4);
        auto p2 = makeProcess("p02", 2, 4);
        s.addProcess(p1);
        s.addProcess(p2);
        for (int k = 0; k < 4; k++) s.tick();
        check("fcfs ignores quantum: p01 done before p02 starts",
              p1->isFinished() && p2->getCurrentLine() == 0);
    }

    // --- test 11: RR alone on a core keeps running with no gaps ---
    {
        Scheduler s(makeConfig(1, "rr", 1));
        auto p1 = makeProcess("p01", 1, 3);
        s.addProcess(p1);
        runUntilDone(s);
        check("rr alone: 3 instructions in 3 ticks",        s.getTicks() == 3);
    }

    // --- test 12: finishing exactly at the quantum boundary -> FINISHED, not re-queued ---
    {
        Scheduler s(makeConfig(1, "rr", 2));
        auto p1 = makeProcess("p01", 1, 2);
        auto p2 = makeProcess("p02", 2, 1);
        s.addProcess(p1);
        s.addProcess(p2);
        s.tick(); s.tick();
        check("rr boundary: p01 FINISHED, not READY",       p1->isFinished());
        runUntilDone(s);
        check("rr boundary: took exactly 3 ticks",          s.getTicks() == 3);
    }

    // --- test 13: sleeping at the quantum boundary -> SLEEPING, not re-queued ---
    {
        Scheduler s(makeConfig(1, "rr", 2));
        auto p1 = makeProcess("p01", 1, { makePrint(), makeSleep(2), makePrint() });
        s.addProcess(p1);
        s.tick(); s.tick();
        check("rr boundary: p01 SLEEPING, not READY",       p1->isSleeping());
        runUntilDone(s);
        check("rr boundary: sleep still counted, 5 ticks",  s.getTicks() == 5);
    }

    // --- test 14: RR + SLEEP with an idle core (the crash case) ---
    {
        Scheduler s(makeConfig(1, "rr", 1));
        auto p1 = makeProcess("p01", 1, { makePrint(), makeSleep(3), makePrint() });
        s.addProcess(p1);
        runUntilDone(s);
        check("rr + sleep + idle core: no crash, finished", p1->isFinished());
    }

    // --- test 15: RR, 2 cores, 3 processes -> preempted process waits behind the queue ---
    {
        Scheduler s(makeConfig(2, "rr", 1));
        auto p1 = makeProcess("p01", 1, 2);
        auto p2 = makeProcess("p02", 2, 2);
        auto p3 = makeProcess("p03", 3, 2);
        s.addProcess(p1);
        s.addProcess(p2);
        s.addProcess(p3);

        s.tick();   // p01 + p02 run once, both preempted; queue: p03, p01, p02
        s.tick();   // p03 + p01 run
        check("rr 2 cores: p03 got a core before p02's 2nd turn",
              p3->getCurrentLine() == 1 && p2->getCurrentLine() == 1);
        check("rr 2 cores: a process runs at most once per tick",
              p1->getCurrentLine() == 2);

        runUntilDone(s);
        check("rr 2 cores: took exactly 3 ticks",           s.getTicks() == 3);
    }

    // --- test 16: delay-per-exec 2, FCFS -> PRINT, wait, wait, PRINT, wait, wait, PRINT ---
    {
        Scheduler s(makeConfig(1, "fcfs", 1, 2));
        auto p1 = makeProcess("p01", 1, 3);
        auto p2 = makeProcess("p02", 2, 1);
        s.addProcess(p1);
        s.addProcess(p2);

        s.tick(); s.tick();   // PRINT, wait
        check("delay: busy-waiting process keeps its core", p1->getCoreID() == 0 && p1->getCurrentLine() == 1);
        check("delay: other process can't use the core",    p2->getCurrentLine() == 0);

        runUntilDone(s);
        check("delay: 3 instructions + 2x2 waits + p02 = 8 ticks", s.getTicks() == 8);
    }

    // --- test 17: delay + RR -> owed delay survives preemption ---
    {
        Scheduler s(makeConfig(1, "rr", 2, 2));
        auto p1 = makeProcess("p01", 1, 2);
        auto p2 = makeProcess("p02", 2, 2);
        s.addProcess(p1);
        s.addProcess(p2);

        s.tick(); s.tick();   // p01: PRINT, wait -> preempted with 1 wait still owed
        check("delay+rr: p01 preempted mid-delay",          p1->getCoreID() == -1 && p1->getDelayLeft() == 1);

        runUntilDone(s);
        check("delay+rr: owed delay is paid, 8 ticks",      s.getTicks() == 8);
    }

    // --- test 18: delay + SLEEP -> no extra busy-wait after waking ---
    {
        Scheduler s(makeConfig(1, "fcfs", 1, 2));
        auto p1 = makeProcess("p01", 1, { makePrint(), makeSleep(1), makePrint() });
        s.addProcess(p1);
        runUntilDone(s);
        // PRINT, wait, wait, SLEEP, (asleep 1 tick), PRINT = 6 ticks
        check("delay+sleep: took exactly 6 ticks",          s.getTicks() == 6);
    }

    return 0;
}
