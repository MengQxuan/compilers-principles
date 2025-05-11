#ifndef __LIVE_VARIABLE_ANALYSIS_H__
#define __LIVE_VARIABLE_ANALYSIS_H__

#include <set>
#include <map>

class MachineFunction;
class MachineUnit;
class MachineOperand;
class MachineBlock;
class LiveVariableAnalysis
{
private:
    // 键是一个 MachineOperand，值是指向 MachineOperand 的指针集合，表示哪些操作数使用了该键操作数
    std::map<MachineOperand, std::set<MachineOperand *>> all_uses; // 记录每个操作数被使用的所有位置（即被哪些指令使用）

    // 键是一个指向 MachineBlock 的指针，值是指向 MachineOperand 的指针集合
    std::map<MachineBlock *, std::set<MachineOperand *>> def, use; // 每个基本块 定义和使用 的操作数集合

    void computeUsePos(MachineFunction *); // 计算每个操作数被使用的位置
    void computeDefUse(MachineFunction *); // 计算每个基本块的定义（def）和使用（use）集合
    void iterate(MachineFunction *);       // 迭代计算每个基本块的 LiveIn 和 LiveOut 集合，直到收敛

public:
    void pass(MachineUnit *unit);                                                            // 对整个机器单元（可能包含多个函数）执行活跃变量分析
    void pass(MachineFunction *func);                                                        // 对单个机器函数执行活跃变量分析
    std::map<MachineOperand, std::set<MachineOperand *>> &getAllUses() { return all_uses; }; // 返回 all_uses 的引用，便于外部访问
};

#endif