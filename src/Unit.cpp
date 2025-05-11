#include "Unit.h"
#include "Type.h"
#include <unordered_map>
#include <deque>
#include <unordered_set>

extern FILE *yyout;

void Unit::insertFunc(Function *f)
{
    func_list.push_back(f);
}

void Unit::removeFunc(Function *func)
{
    func_list.erase(std::find(func_list.begin(), func_list.end(), func));
}

void Unit::output() const
{
    fprintf(yyout, "target triple = \"x86_64-pc-linux-gnu\"\n\n");
    // fprintf(yyout, "target triple = \"armv7-unknown-linux-gnueabihf\"\n\n");
    fprintf(yyout, "declare i32 @getint()\n");
    fprintf(yyout, "declare void @putint(i32)\n");
    fprintf(yyout, "declare i32 @getch()\n");
    fprintf(yyout, "declare void @putch(i32)\n");
    fprintf(yyout, "declare void @putf(i32)\n\n");
    for (auto i : global_var)
        i->output();
    for (auto &func : func_list)
        func->output();
}
void Unit::initLibraryFunctions()
{
    // printf("%s\n", argv[optind]);
    Type *funcType;
    funcType = new FunctionType(TypeSystem::intType, {});
    SymbolEntry *se = new IdentifierSymbolEntry(funcType, "getint", identifiers->getLevel());
    identifiers->install("getint", se);

    // getch
    funcType = new FunctionType(TypeSystem::intType, {});
    se = new IdentifierSymbolEntry(funcType, "getch", identifiers->getLevel());
    identifiers->install("getch", se);

    // getfloat
    funcType = new FunctionType(TypeSystem::floatType, {});
    se = new IdentifierSymbolEntry(funcType, "getfloat", identifiers->getLevel());
    identifiers->install("getfloat", se);

    // // getarray
    // funcType = new FunctionType(TypeSystem::intType, {});
    // se = new IdentifierSymbolEntry(funcType, "getarray", identifiers->getLevel());
    // identifiers->install("getarray", se);

    // putint
    funcType = new FunctionType(TypeSystem::voidType, {TypeSystem::intType});
    se = new IdentifierSymbolEntry(funcType, "putint", identifiers->getLevel());
    identifiers->install("putint", se);

    // putch
    funcType = new FunctionType(TypeSystem::voidType, {TypeSystem::intType});
    se = new IdentifierSymbolEntry(funcType, "putch", identifiers->getLevel());
    identifiers->install("putch", se);

    // putfloat
    funcType = new FunctionType(TypeSystem::voidType, {TypeSystem::floatType});
    se = new IdentifierSymbolEntry(funcType, "putfloat", identifiers->getLevel());
    identifiers->install("putfloat", se);

    // // putarray
    // funcType = new FunctionType(TypeSystem::voidType, {});
    // se = new IdentifierSymbolEntry(funcType, "putarray", identifiers->getLevel());
    // identifiers->install("putarray", se);

    // putf
    funcType = new FunctionType(TypeSystem::voidType, {});
    se = new IdentifierSymbolEntry(funcType, "putf", identifiers->getLevel());
    identifiers->install("putf", se);

    // starttime
    funcType = new FunctionType(TypeSystem::voidType, {});
    se = new IdentifierSymbolEntry(funcType, "starttime", identifiers->getLevel());
    identifiers->install("starttime", se);

    // stoptime
    funcType = new FunctionType(TypeSystem::voidType, {});
    se = new IdentifierSymbolEntry(funcType, "stoptime", identifiers->getLevel());
    identifiers->install("stoptime", se);
}
void Unit::removeUnusedAlloca()
{
    std::cout << "Removing unused alloca instruction" << std::endl;

    // 遍历所有函数
    for (auto &func : func_list)
    {
        // 存储与 ALLOCA 操作数相关的 STORE 指令
        std::unordered_map<Operand *, std::vector<Instruction *>> storeInstructions;
        // 遍历函数中的每个基本块
        for (auto &block : func->getBlockList())
        {
            // 第一次遍历：记录 STORE 指令与 ALLOCA 操作数的关系
            for (Instruction *inst = block->begin(); inst != block->end(); inst = inst->getNext())
            {
                if (inst->isStore())
                {
                    Operand *storeOp = inst->getUse()[0];       // 获取 STORE 指令的第一个操作数
                    storeInstructions[storeOp].push_back(inst); // 将 STORE 指令存储到对应的操作数中
                }
            }
        }
        for (auto &block : func->getBlockList())
        { // 第二次遍历：删除未使用的 ALLOCA 指令及其相关的 STORE 指令
            for (Instruction *inst = block->begin(); inst != block->end(); inst = inst->getNext())
            {
                if (inst->isAlloca())
                {
                    Operand *allocaOp = inst->getDef(); // 获取 ALLOCA 指令的定义操作数
                    // std::cout << "Operand users count: " << allocaOp->loadusersNum() << std::endl;

                    // 如果 ALLOCA 操作数没有任何使用者，则删除 ALLOCA 指令
                    if (allocaOp && allocaOp->loadusersNum() == 0)
                    {
                        // std::cout << "Unused alloca instruction found, removing..." << std::endl;

                        // 删除 ALLOCA 指令
                        block->remove(inst);
                        delete inst;

                        // 获取与该 ALLOCA 指令相关的所有 STORE 指令
                        auto storeIt = storeInstructions.find(allocaOp);
                        if (storeIt != storeInstructions.end())
                        {
                            // 删除相关的 STORE 指令
                            for (auto *storeInst : storeIt->second)
                            {
                                block->remove(storeInst); // 删除相关的 STORE 指令
                                // std::cout << "Removed related store instruction" << std::endl;
                            }
                        }
                    }
                }
            }
        }
    }
}

void Unit::samebboptimize()
{
    std::cout << "Starting samebb optimization..." << std::endl;

    // 遍历所有函数
    for (auto &func : func_list)
    {
        // 遍历函数中的每个基本块
        for (auto &block : func->getBlockList())
        {
            // fprintf(stderr, "Block: %d\n", block->getNo());
            //  第一阶段：收集所有需要优化的 alloca 指令
            std::vector<AllocaInstruction *> allocas;
            for (Instruction *inst = block->begin(); inst != block->end(); inst = inst->getNext())
            {
                // fprintf(stderr, "Instruction: %d\n", inst->getInstType());
                if (inst->isAlloca())
                {
                    // fprintf(stderr, "Found alloca instruction: %d\n", inst->getInstType());
                    AllocaInstruction *allocaInst = dynamic_cast<AllocaInstruction *>(inst);
                    if (!allocaInst)
                        continue;

                    Operand *defOperand = allocaInst->getDef();
                    if (!defOperand)
                        continue;

                    bool allUsesInSameBB = true;
                    for (auto userInst : defOperand->getUse())
                    {
                        if (userInst->getParent() != block)
                        {
                            allUsesInSameBB = false;
                            break;
                        }
                    }

                    // fprintf(stderr, "All uses in same BB: %d\n", allUsesInSameBB);
                    if (allUsesInSameBB)
                    {
                        allocas.push_back(allocaInst);
                    }
                }
            }

            if (allocas.empty())
            {
                fprintf(stderr, "No alloca to optimize in this block\n");
                continue; // 当前基本块没有需要优化的 alloca
            }

            // 准备数据结构保存替换和删除的信息
            // 对于每个 AllocaInstruction，维护一个当前值 Operand*
            std::unordered_map<AllocaInstruction *, Operand *> currentVals;
            // 记录 LoadInstruction 需要替换的 Operand
            std::unordered_map<LoadInstruction *, Operand *> loadReplacements;
            // 标记要删除的 Store 和 Load 指令
            std::unordered_set<StoreInstruction *> storesToDelete;
            std::unordered_set<LoadInstruction *> loadsToDelete;

            // std::unordered_map<Operand *, Operand *> Vals;  //def -> val
            // 第二阶段：遍历基本块，处理所有的 alloca
            for (Instruction *inst = block->begin(); inst != block->end(); inst = inst->getNext())
            {
                // 处理 StoreInstruction
                if (inst->isStore())
                {
                    StoreInstruction *storeInst = dynamic_cast<StoreInstruction *>(inst);
                    if (!storeInst)
                        continue;

                    Operand *storeAddr = storeInst->getUse()[0];
                    Operand *storeVal = storeInst->getUse()[1];

                    // 检查 storeAddr 是否是我们要优化的 alloca 的 Operand
                    for (auto allocaInst : allocas)
                    {
                        Operand *allocaOperand = allocaInst->getDef();
                        if (storeAddr == allocaOperand)
                        {
                            // 更新 currentVal
                            currentVals[allocaInst] = storeVal;
                            // 标记 store 指令删除
                            storesToDelete.insert(storeInst);
                            break; // 一个 store 指令只能对应一个 alloca
                        }
                    }
                }
                // 处理 LoadInstruction
                else if (inst->isLoad())
                {
                    LoadInstruction *loadInst = dynamic_cast<LoadInstruction *>(inst);
                    if (!loadInst)
                        continue;

                    Operand *loadAddr = loadInst->getUse()[0];
                    // Operand *loadDef = loadInst->getDef();

                    // 检查 loadAddr 是否是我们要优化的 alloca 的 Operand
                    for (auto allocaInst : allocas)
                    {
                        Operand *allocaOperand = allocaInst->getDef();
                        if (loadAddr == allocaOperand)
                        {
                            // 记录替换信息
                            loadReplacements[loadInst] = currentVals[allocaInst];
                            // 标记 load 指令删除
                            loadsToDelete.insert(loadInst);
                            break; // 一个 load 指令只能对应一个 alloca
                        }
                    }
                }
            }

            // 第三阶段：进行寄存器的替换
            for (auto &pair : loadReplacements)
            {
                LoadInstruction *loadInst = pair.first;
                Operand *replacementVal = pair.second;

                Operand *loadDef = loadInst->getDef();
                if (!loadDef)
                    continue;

                // 获取 loadDef 的所有使用者
                std::vector<Instruction *> users = loadDef->getUse();

                for (auto userInst : users)
                {
                    // fprintf(stderr, "当前指令类型是：%d\n", userInst->getInstType());
                    userInst->replaceUse(loadDef, replacementVal);
                    // fprintf(stderr, "Replaced %s with %s\n", loadDef->toStr().c_str(), replacementVal->toStr().c_str());
                    // fprintf(stderr, "User instruction: %s\n", userInst->getDef()->toStr().c_str());
                    //  fprintf(stderr, "User instruction: %s\n", userInst->getUse()[0]->toStr().c_str());
                }
            }

            // 第四阶段：删除标记的指令
            for (auto storeInst : storesToDelete)
            {
                block->remove(storeInst);
                delete storeInst;
            }
            for (auto loadInst : loadsToDelete)
            {
                block->remove(loadInst);
                delete loadInst;
            }

            // 第五阶段：删除未使用的 alloca 指令
            std::cout << "Deleting unused alloca instructions..." << std::endl;
            for (auto allocaInst : allocas)
            {
                Operand *allocaOperand = allocaInst->getDef();
                if (allocaOperand->getUse().empty())
                {
                    block->remove(allocaInst);
                    delete allocaInst;
                }
                else
                {
                    std::cout << "Alloca instruction still has uses, not deleting" << endl;
                }
            }
        }
    }
    std::cout << "samebb optimization completed." << std::endl;
}
Unit::~Unit()
{
    auto delete_var = global_var;
    for (auto &i : delete_var)
        delete i;
    auto delete_list = func_list;
    for (auto &func : delete_list)
        delete func;
}
void Unit::genMachineCode(MachineUnit* munit) 
{
    AsmBuilder* builder = new AsmBuilder();
    builder->setUnit(munit);
    for (auto &func : func_list)
        func->genMachineCode(builder);
}