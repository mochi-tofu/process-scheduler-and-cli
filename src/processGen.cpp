#include "processGen.h"

//variable names the random instructions pick from
//small pool so ADD/SUBTRACT often reuse variables that were already declared
static const std::vector<std::string> VAR_NAMES = {"x", "y", "z", "a", "b"};

ProcessGenerator::ProcessGenerator(const Config& con): config(con), rng(std::random_device{}()) {}

ProcessGenerator::ProcessGenerator(const Config& con, uint64_t seed): config(con), rng(seed) {}

uint64_t ProcessGenerator::randBetween(uint64_t lo, uint64_t hi){
    std::uniform_int_distribution<uint64_t> dist(lo, hi);
    return dist(rng);
}

std::string ProcessGenerator::randomVar(){
    return VAR_NAMES[randBetween(0, VAR_NAMES.size() - 1)];
}

//operand is either a variable or a uint16 value, 50/50
Operand ProcessGenerator::randomOperand(){
    Operand op;
    if (randBetween(0, 1)){
        op.isVariable = true;
        op.name = randomVar();
    }
    else{
        op.value = randBetween(0, 65535);
    }
    return op;
}

//one instruction that isn't a FOR
Instruction ProcessGenerator::randomSimple(){
    Instruction ins;

    switch (randBetween(0, 4)){
        case 0:
            ins.type = InstructionType::PRINT;
            break;
        case 1:
            ins.type = InstructionType::DECLARE;
            ins.target = randomVar();
            ins.value = randBetween(0, 65535);
            break;
        case 2:
            ins.type = InstructionType::ADD;
            ins.target = randomVar();
            ins.operand1 = randomOperand();
            ins.operand2 = randomOperand();
            break;
        case 3:
            ins.type = InstructionType::SUBTRACT;
            ins.target = randomVar();
            ins.operand1 = randomOperand();
            ins.operand2 = randomOperand();
            break;
        case 4:
            ins.type = InstructionType::SLEEP;
            ins.value = randBetween(1, 255); //spec: X is uint8
            break;
    }
    return ins;
}

//returns EXACTLY count instructions, with FOR loops already expanded (flattened)
//depth = how many FORs we are inside of right now (0 = top level of the process)
std::vector<Instruction> ProcessGenerator::generateBlock(uint64_t count, int depth){
    std::vector<Instruction> block;

    while (block.size() < count){
        uint64_t remaining = count - block.size();

        //FOR needs room for at least 2 instructions (body of 1, repeated 2 times)
        //and we can't go deeper than MAX_FOR_DEPTH
        //randBetween(0, 5) == 0 -> about 1 in 6 picks is a FOR, like each of the 5 simple types
        if (depth < MAX_FOR_DEPTH && remaining >= 2 && randBetween(0, 5) == 0){

            //pick repeats first, then a body small enough that body x repeats fits in what's left
            uint64_t repeats = randBetween(2, std::min(MAX_REPEATS, remaining));
            uint64_t bodySize = randBetween(1, std::min(MAX_BODY, remaining / repeats));

            //the body can contain its own FORs, one level deeper
            std::vector<Instruction> body = generateBlock(bodySize, depth + 1);

            //expansion: copy the body in once per repeat
            for (uint64_t r = 0; r < repeats; r++){
                block.insert(block.end(), body.begin(), body.end());
            }
        }

        else{
            block.push_back(randomSimple());
        }
    }

    return block;
}

std::shared_ptr<Process> ProcessGenerator::generate(const std::string& name, uint64_t id){
    auto p = std::make_shared<Process>(name, id);

    uint64_t total = randBetween(config.minIns, config.maxIns);
    for (const Instruction& ins : generateBlock(total, 0)){
        p->addInstruction(ins);
    }

    return p;
}
