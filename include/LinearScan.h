/**
 * linear scan register allocation
 */

#ifndef _LINEARSCAN_H__
#define _LINEARSCAN_H__
#include <set>
#include <map>
#include <vector>
#include <list>

class MachineUnit;
class MachineOperand;
class MachineFunction;

class LinearScan
{
private:
    struct Interval
    {
        int start;                       // 活跃区间的起始位置（指令编号）
        int end;                         // 活跃区间的结束位置（指令编号）
        bool spill;                      // 是否需要溢出到内存 whether this vreg should be spilled to memory
        int disp;                        // 在堆栈中的偏移量 displacement in stack
        int rreg;                        // 映射到的物理寄存器编号，如果未溢出 the real register mapped from virtual register if the vreg is not spilled to memory
        std::set<MachineOperand *> defs; // 定义该寄存器的所有操作数
        std::set<MachineOperand *> uses; // 使用该寄存器的所有操作数
    };
    MachineUnit *unit;
    MachineFunction *func;
    std::vector<int> regs;                                            // 可用的物理寄存器编号
    std::map<MachineOperand *, std::set<MachineOperand *>> du_chains; // 定义-使用链，记录每个定义操作数被哪些使用操作数引用
    std::vector<Interval *> intervals;                                // 活跃区间列表

    static bool compareStart(Interval *a, Interval *b); // 比较函数，根据活跃区间的起始位置排序
    void expireOldIntervals(Interval *interval);        // 释放那些已经结束的活跃区间，回收其占用的寄存器
    void spillAtInterval(Interval *interval);           // 在当前活跃区间上进行溢出处理，将其存储到内存
    void makeDuChains();                                // 构建定义-使用链，记录每个定义操作数被哪些使用操作数引用
    void computeLiveIntervals();                        // 计算每个虚拟寄存器的活跃区间
    bool linearScanRegisterAllocation();                // 执行线性扫描寄存器分配，尝试将虚拟寄存器映射到物理寄存器
    void modifyCode();                                  // 修改机器代码以反映寄存器分配结果，将虚拟寄存器替换为物理寄存器
    void genSpillCode();                                // 生成溢出代码（加载和存储指令），处理需要溢出的寄存器

    static bool compareEnd(Interval *a, Interval *b); // 比较函数，根据活跃区间的结束位置排序
    std::vector<Interval *> actives;                  // 当前活跃的区间列表

public:
    LinearScan(MachineUnit *unit);
    void allocateRegisters(); // 执行寄存器分配
};

#endif