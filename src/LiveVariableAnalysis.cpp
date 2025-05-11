#include "LiveVariableAnalysis.h"
#include "MachineCode.h"
#include <algorithm>

// 对整个机器单元中的每个函数执行活跃变量分析
void LiveVariableAnalysis::pass(MachineUnit *unit)
{
    for (auto &func : unit->getFuncs())
    {
        computeUsePos(func); // 计算操作数的使用位置
        computeDefUse(func); // 计算每个基本块的 def 和 use 集合
        iterate(func);       // 迭代计算 LiveIn 和 LiveOut 集合，直到收敛
    }
}

// 对单个机器函数执行活跃变量分析
void LiveVariableAnalysis::pass(MachineFunction *func)
{
    computeUsePos(func);
    computeDefUse(func);
    iterate(func);
}

// 计算每个基本块的定义 (def) 和使用 (use) 集合
void LiveVariableAnalysis::computeDefUse(MachineFunction *func)
{
    for (auto &block : func->getBlocks()) // 遍历函数中的每个基本块
    {
        // 遍历基本块中的每条指令
        for (auto inst = block->getInsts().begin(); inst != block->getInsts().end(); inst++)
        {
            auto user = (*inst)->getUse();                             // 获取指令使用的所有操作数（inst->getUse()）
            std::set<MachineOperand *> temp(user.begin(), user.end()); // 创建一个临时集合 temp，包含所有使用的操作数

            // use[block] 由 temp 与 def[block] 的差集组成
            // 即：在基本块中使用但未定义的操作数
            set_difference(temp.begin(), temp.end(),
                           def[block].begin(), def[block].end(), inserter(use[block], use[block].end()));

            // 将 all_uses[*d] 中的所有使用位置插入到 def[block] 中。
            // 即：记录哪些操作数被定义了。
            auto defs = (*inst)->getDef();
            for (auto &d : defs)
                def[block].insert(all_uses[*d].begin(), all_uses[*d].end());
        }
    }
}

// 通过迭代算法计算每个基本块的 LiveIn 和 LiveOut 集合，直到所有集合不再变化（即收敛）
void LiveVariableAnalysis::iterate(MachineFunction *func)
{
    // 遍历函数中的每个基本块，清空其 LiveIn 集合
    for (auto &block : func->getBlocks())
        block->getLiveIn().clear();
    bool change;
    change = true;
    while (change) // 如果在一次完整的遍历中，任何一个基本块的 LiveIn 集合发生变化，继续迭代
    {
        change = false;
        for (auto &block : func->getBlocks())
        {
            block->getLiveOut().clear();   // 清空 LiveOut
            auto old = block->getLiveIn(); // 保存旧的 LiveIn

            // 计算 LiveOut
            for (auto &succ : block->getSuccs()) // 遍历所有后继基本块
                                                 // 将后继基本块的 LiveIn 集合合并到当前基本块的 LiveOut 集合中
                block->getLiveOut().insert(succ->getLiveIn().begin(), succ->getLiveIn().end());

            // 计算 LiveIn
            block->getLiveIn() = use[block]; // 初始化 LiveIn 为 use[block]
            // 通过 set_difference，从 LiveOut 中减去 def[block]，并将结果加入到 LiveIn 中。
            // 即：LiveIn = use[block] ∪ (LiveOut - def[block])
            std::vector<MachineOperand *> temp;
            set_difference(block->getLiveOut().begin(), block->getLiveOut().end(),
                           def[block].begin(), def[block].end(), inserter(block->getLiveIn(), block->getLiveIn().end()));

            if (old != block->getLiveIn())
                change = true; // 如果 LiveIn 集合发生变化（old != block->getLiveIn()），设置 change = true，以继续迭代
        }
    }
}

// 计算每个操作数被使用的位置，填充 all_uses
void LiveVariableAnalysis::computeUsePos(MachineFunction *func)
{
    for (auto &block : func->getBlocks()) // 遍历函数中的每个基本块
    {
        for (auto &inst : block->getInsts()) // 遍历基本块中的每条指令
        {
            auto uses = inst->getUse(); // 获取操作数的使用位置
            for (auto &use : uses)
                all_uses[*use].insert(use); // 将操作数 *use 的使用位置添加到集合中
        }
    }
}
