#include "processGen.h"
#include "scheduler.h"
#include <iostream>
#include <set>

Config makeConfig(uint64_t minIns, uint64_t maxIns) {
    Config c;
    c.numCpu = 1;
    c.scheduler = "fcfs";
    c.quantumCycles = 1;
    c.delayPerExec = 0;
    c.minIns = minIns;
    c.maxIns = maxIns;
    return c;
}

void check(const std::string& label, bool ok) {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << label << "\n";
}

int main() {
    // --- test 1: length always within [min-ins, max-ins], over many processes ---
    {
        ProcessGenerator gen(makeConfig(1000, 2000), 42);
        bool allInRange = true;
        for (int k = 0; k < 300; k++) {
            auto p = gen.generate("p", k);
            size_t n = p->getInsList().size();
            if (n < 1000 || n > 2000) allInRange = false;
        }
        check("length: 300 processes all within [1000, 2000]", allInRange);
    }

    // --- test 2: min-ins == max-ins gives exactly that many, even with FOR expansion ---
    {
        ProcessGenerator gen(makeConfig(37, 37), 7);
        bool allExact = true;
        for (int k = 0; k < 300; k++) {
            if (gen.generate("p", k)->getInsList().size() != 37) allExact = false;
        }
        check("length: min == max == 37 gives exactly 37 every time", allExact);
    }

    // --- test 3: tiny sizes (1 and 2) work: no room or barely room for a FOR ---
    {
        ProcessGenerator gen(makeConfig(1, 2), 3);
        bool ok = true;
        for (int k = 0; k < 300; k++) {
            size_t n = gen.generate("p", k)->getInsList().size();
            if (n < 1 || n > 2) ok = false;
        }
        check("length: min 1, max 2 stays within range", ok);
    }

    // --- test 4: every instruction type shows up, and nothing is NONE ---
    {
        ProcessGenerator gen(makeConfig(1000, 1000), 1);
        auto p = gen.generate("p01", 1);
        std::set<InstructionType> seen;
        for (const auto& ins : p->getInsList()) seen.insert(ins.type);
        check("types: PRINT, DECLARE, ADD, SUBTRACT, SLEEP all appear",
              seen.count(InstructionType::PRINT) && seen.count(InstructionType::DECLARE) &&
              seen.count(InstructionType::ADD) && seen.count(InstructionType::SUBTRACT) &&
              seen.count(InstructionType::SLEEP));
        check("types: no NONE instructions generated", !seen.count(InstructionType::NONE));
    }

    // --- test 5: SLEEP values fit uint8 ---
    {
        ProcessGenerator gen(makeConfig(1000, 1000), 5);
        bool ok = true;
        auto p = gen.generate("p", 1);   // keep p alive: looping over generate(...)->getInsList() directly would dangle
        for (const auto& ins : p->getInsList()) {
            if (ins.type == InstructionType::SLEEP && ins.value > 255) ok = false;
        }
        check("sleep: all SLEEP values are <= 255", ok);
    }

    // --- test 6: same seed gives the same process (tests are repeatable) ---
    {
        ProcessGenerator a(makeConfig(100, 200), 99);
        ProcessGenerator b(makeConfig(100, 200), 99);
        auto pa = a.generate("p", 1);
        auto pb = b.generate("p", 1);
        bool same = pa->getInsList().size() == pb->getInsList().size();
        for (size_t i = 0; same && i < pa->getInsList().size(); i++) {
            if (pa->getInsList()[i].type != pb->getInsList()[i].type) same = false;
        }
        check("seed: same seed gives the same instructions", same);
    }

    // --- test 7: name and ID are passed through ---
    {
        ProcessGenerator gen(makeConfig(10, 10), 1);
        auto p = gen.generate("p07", 7);
        check("name/id: p07 and 7", p->getName() == "p07" && p->getID() == 7);
    }

    // --- test 8: a generated process actually runs to FINISHED in the scheduler ---
    {
        Config c = makeConfig(50, 100);
        ProcessGenerator gen(c, 11);
        Scheduler s(c);
        auto p = gen.generate("p01", 1);
        s.addProcess(p);
        for (int t = 0; t < 100000 && !p->isFinished(); t++) s.tick();
        check("run: generated process finishes", p->isFinished());
        check("run: every instruction executed", p->getCurrentLine() == p->getInsList().size());
    }

    // ===== scheduler-start hook (generation inside tick) =====

    // --- test 9: no generation until startGenerating(), then one every batch-process-freq ticks ---
    {
        Config c = makeConfig(5, 5);
        c.batchProcessFreq = 3;
        Scheduler s(c);
        for (int t = 0; t < 6; t++) s.tick();
        check("gen: nothing generated before scheduler-start", s.getAllProcesses().empty());

        s.startGenerating();
        for (int t = 0; t < 9; t++) s.tick();   // ticks 7..15 -> generated at end of ticks 9, 12, 15
        check("gen: freq 3 over 9 ticks makes 3 processes", s.getAllProcesses().size() == 3);

        s.stopGenerating();
        for (int t = 0; t < 9; t++) s.tick();
        check("gen: scheduler-stop stops generating", s.getAllProcesses().size() == 3);
    }

    // --- test 10: freq 1 = one per tick; names p01, p02, ... and unique IDs ---
    {
        Config c = makeConfig(5, 5);
        c.batchProcessFreq = 1;
        Scheduler s(c);
        s.startGenerating();
        for (int t = 0; t < 12; t++) s.tick();
        auto all = s.getAllProcesses();
        check("gen: freq 1 makes one per tick", all.size() == 12);
        check("gen: names p01, p09, p10, p12",
              all[0]->getName() == "p01" && all[8]->getName() == "p09" &&
              all[9]->getName() == "p10" && all[11]->getName() == "p12");
        std::set<uint64_t> ids;
        for (auto& p : all) ids.insert(p->getID());
        check("gen: all IDs unique", ids.size() == 12);
    }

    // --- test 11: screen -s style createProcess: duplicate names refused, auto names skip taken ones ---
    {
        Config c = makeConfig(5, 5);
        c.batchProcessFreq = 1;
        Scheduler s(c);
        check("create: new name works", s.createProcess("p02") != nullptr);
        check("create: duplicate name refused", s.createProcess("p02") == nullptr);

        s.startGenerating();
        s.tick();
        s.tick();
        auto all = s.getAllProcesses();
        check("gen: auto names skip p02 (taken) -> p01, p03",
              all.size() == 3 && all[1]->getName() == "p01" && all[2]->getName() == "p03");
    }

    // --- test 12: getProcessInfo / getReport snapshots ---
    {
        Config c = makeConfig(3, 3);
        c.numCpu = 2;
        Scheduler s(c);
        // fixed 3-PRINT processes (random ones might SLEEP and leave the core, making the checks flaky)
        for (std::string name : {"a", "b", "c"}) {
            auto p = std::make_shared<Process>(name, 0);
            for (int k = 0; k < 3; k++) {
                Instruction i;
                i.type = InstructionType::PRINT;
                p->addInstruction(i);
            }
            s.addProcess(p);
        }
        s.tick();

        SystemReport r = s.getReport();
        check("report: 2 cores, both used", r.totalCores == 2 && r.coresUsed == 2);
        check("report: a and b running, c waiting (not listed)",
              r.running.size() == 2 && r.running[0].name == "a" && r.running[1].name == "b");

        ProcessInfo info;
        check("info: unknown name returns false", !s.getProcessInfo("nope", info));
        check("info: a found, 3 lines of code",
              s.getProcessInfo("a", info) && info.totalLines == 3 && info.coreID == 0);

        for (int t = 0; t < 2000 && s.getReport().finished.size() < 3; t++) s.tick();
        r = s.getReport();
        check("report: all 3 finished, none running", r.finished.size() == 3 && r.running.empty());
        s.getProcessInfo("a", info);
        check("info: finished process still found, marked finished", info.finished);
    }

    return 0;
}
