#include "Function.h"
#include "Unit.h"
#include "Type.h"
#include <list>

extern FILE* yyout;

Function::Function(Unit *u, SymbolEntry *s)
{
    u->insertFunc(this);//将函数加入到unit的函数列表中
    entry = new BasicBlock(this);//创建entry入口块
    sym_ptr = s;
    parent = u;
}

Function::~Function()
{
    auto delete_list = block_list;
    for (auto &i : delete_list)
        delete i;
    parent->removeFunc(this);
}
void FuncCallInstruction::output() const
{
    std::string dstr, type, paramStr;
    dstr = dst->toStr();
    type = ((FunctionType *)se->getType())->getRetType()->toStr();
    if (params.size() == 0)
    {
        paramStr = "";
    }
    else
    {
        paramStr += params[0]->getType()->toStr() + " " + params[0]->toStr();
        for (unsigned int i = 1; i < params.size(); i++)
        {
            paramStr += ", " + params[i]->getType()->toStr() + " " + params[i]->toStr();
        }
    }
    if (((FunctionType *)se->getType())->getRetType()->isVoid())
    {
        fprintf(yyout, "  call %s %s(%s)\n", type.c_str(), se->toStr().c_str(), paramStr.c_str());
    }
    else
        fprintf(yyout, "  %s = call %s %s(%s)\n", dstr.c_str(), type.c_str(), se->toStr().c_str(), paramStr.c_str());
}
// remove the basicblock bb from its block_list.
void Function::remove(BasicBlock *bb)
{
    block_list.erase(std::find(block_list.begin(), block_list.end(), bb));
}

void Function::output() const
{
    fprintf(stderr, "\n");
    FunctionType *funcType = dynamic_cast<FunctionType *>(sym_ptr->getType());
    Type *retType = funcType->getRetType();
    std::string paramsStr = "";
    if (paramsSymPtr.size() != 0)
    {
        for (auto p : paramsSymPtr)
        {
            paramsStr += p->getType()->toStr() + " " + ((IdentifierSymbolEntry *)p)->getAddr()->toStr() + ", ";
        }
        paramsStr = paramsStr.substr(0, paramsStr.size() - 2);
    }

    // 输出函数定义头
    fprintf(yyout, "define %s %s(%s) {\n", retType->toStr().c_str(), sym_ptr->toStr().c_str(), paramsStr.c_str());

    // 使用 DFS 标记访问过的基本块
    std::set<BasicBlock *> visitedBlocks;
    
    // 从入口块开始深度优先遍历，消除不可达基本块
    dfs1(entry, visitedBlocks);

    // 输出已访问的基本块
    for (auto bb : visitedBlocks)
    {
        bb->output();  // 输出已访问的基本块
    }

    fprintf(yyout, "}\n");
}

// 使用深度优先搜索（DFS）遍历基本块，消除不可达基本块
void Function::dfs1(BasicBlock *block, std::set<BasicBlock *> &visitedBlocks) const
{
    // 如果已经访问过，返回
    if (visitedBlocks.find(block) != visitedBlocks.end())
        return;

    // 标记当前基本块为已访问
    visitedBlocks.insert(block);

    // 遍历该基本块的所有后继基本块
    for (auto succ = block->succ_begin(); succ != block->succ_end(); ++succ)
    {
        // 递归访问未访问过的后继基本块
        if (visitedBlocks.find(*succ) == visitedBlocks.end())
        {
            dfs1(*succ, visitedBlocks);
        }
    }

    // 遍历该基本块的所有指令，检查跳转/返回指令
    auto inst = block->begin(); // 获取当前基本块的第一条指令
    while (inst != block->end()) 
    {
        // 如果是跳转指令或返回指令
        if (inst->isJump() || inst->isReturn()) 
        {
            // 删除跳转指令后的所有指令
            auto nextInst = inst->getNext();
            while (nextInst != block->end()) 
            {
                auto tempInst = nextInst;
                nextInst = nextInst->getNext();
                block->remove(tempInst); // 删除指令
            }
            break;
        }
        inst = inst->getNext(); // 获取下一条指令
    }
}


void Function::genMachineCode(AsmBuilder* builder) 
{
    auto cur_unit = builder->getUnit();
    auto cur_func = new MachineFunction(cur_unit, this->sym_ptr);
    builder->setFunction(cur_func);
    std::map<BasicBlock*, MachineBlock*> map;
    for(auto block : block_list)
    {
        block->genMachineCode(builder);
        map[block] = builder->getBlock();
    }
    // Add pred and succ for every block
    for(auto block : block_list)
    {
        auto mblock = map[block];
        for (auto pred = block->pred_begin(); pred != block->pred_end(); pred++)
            mblock->addPred(map[*pred]);
        for (auto succ = block->succ_begin(); succ != block->succ_end(); succ++)
            mblock->addSucc(map[*succ]);
    }
    cur_unit->InsertFunc(cur_func);

}
