#include <algorithm>
#include "LinearScan.h"
#include "MachineCode.h"
#include "LiveVariableAnalysis.h"

LinearScan::LinearScan(MachineUnit *unit)
{
    this->unit = unit;
    for (int i = 4; i < 11; i++) // 通用寄存器，ARM 架构中的 r4 到 r10
        regs.push_back(i);
}

// 执行整个寄存器分配过程
void LinearScan::allocateRegisters()
{
    for (auto &f : unit->getFuncs()) // 遍历 MachineUnit 中的每个 MachineFunction
    {
        func = f;
        bool success;
        success = false;
        // 对每个函数，重复以下步骤，直到成功将所有虚拟寄存器映射到物理寄存器
        while (!success) // repeat until all vregs can be mapped
        {
            computeLiveIntervals();                   // 计算当前函数中所有虚拟寄存器的活跃区间
            success = linearScanRegisterAllocation(); // 执行线性扫描寄存器分配，尝试将虚拟寄存器映射到物理寄存器
            if (success)                              // all vregs can be mapped to real regs
                modifyCode();                         // 分配成功，修改机器代码，将虚拟寄存器替换为物理寄存器
            else                                      // spill vregs that can't be mapped to real regs
                genSpillCode();                       // 分配失败，生成溢出代码，将部分虚拟寄存器存储到内存中，并重新尝试分配
        }
    }
}

// 构建定义-使用（Def-Use）链，记录每个定义操作数被哪些使用操作数引用
void LinearScan::makeDuChains()
{
    LiveVariableAnalysis lva;
    lva.pass(func);    // 对当前函数执行活跃变量分析
    du_chains.clear(); // 清空 du_chains，准备重新构建 Def-Use 链
    int i = 0;
    std::map<MachineOperand, std::set<MachineOperand *>> liveVar;
    for (auto &bb : func->getBlocks()) // 对每个基本块bb
    {
        liveVar.clear();                 // 清空 liveVar
        for (auto &t : bb->getLiveOut()) // 遍历 bb 的 LiveOut 集合，记录每个操作数的活跃位置
            liveVar[*t].insert(t);
        int no;
        // no表示当前基本块最后一条指令的编号，i表示所有之前的基本块+当前基本块的指令数量
        no = i = bb->getInsts().size() + i; // 计算指令编号 no，根据基本块中指令数量更新 i 和 no
        for (auto inst = bb->getInsts().rbegin(); inst != bb->getInsts().rend(); inst++)
        {                         // 逆序遍历基本块中的指令（从后向前），以便构建 Def-Use 链
            (*inst)->setNo(no--); // 为每条指令设置编号 no，并递减 no
            for (auto &def : (*inst)->getDef())
            {
                if (def->isVReg()) // 如果 def 是虚拟寄存器
                {
                    auto &uses = liveVar[*def];                      // 获取 def 的所有使用操作数 uses
                    du_chains[def].insert(uses.begin(), uses.end()); // 将 uses 插入到 du_chains[def] 中
                    auto &kill = lva.getAllUses()[*def];             // 获取 def 的所有被杀死的使用操作数 kill
                    std::set<MachineOperand *> res;
                    // 从 uses 中移除被杀死的操作数
                    set_difference(uses.begin(), uses.end(), kill.begin(), kill.end(), inserter(res, res.end()));
                    liveVar[*def] = res; // res = uses - kill
                }
            }
            for (auto &use : (*inst)->getUse())
            {
                if (use->isVReg())
                    liveVar[*use].insert(use); // 对每个使用的操作数 use，如果 use 是虚拟寄存器，记录其在 liveVar 中的使用位置
            }
        }
    }
}

// 计算所有虚拟寄存器的活跃区间，并对区间进行合并和排序
void LinearScan::computeLiveIntervals()
{
    makeDuChains(); // 构建定义-使用链

    // 初始化活跃区间
    intervals.clear();
    for (auto &du_chain : du_chains)
    { // 遍历 du_chains，为每个定义操作数创建一个初始的 Interval
        int t = -1;
        for (auto &use : du_chain.second)
            t = std::max(t, use->getParent()->getNo());
        Interval *interval = new Interval({du_chain.first->getParent()->getNo(), t, false, 0, 0, {du_chain.first}, du_chain.second});
        intervals.push_back(interval);
    }

    // 调整活跃区间
    // 遍历每个 Interval，根据基本块的 LiveIn 和 LiveOut 集合调整其 start 和 end
    // 逻辑较为复杂，主要基于操作数在不同基本块中的活跃状态，调整活跃区间的范围
    for (auto &interval : intervals)
    {
        auto uses = interval->uses;
        auto begin = interval->start;
        auto end = interval->end;
        for (auto block : func->getBlocks())
        {
            auto liveIn = block->getLiveIn();
            auto liveOut = block->getLiveOut();
            bool in = false;
            bool out = false;
            for (auto use : uses)
                if (liveIn.count(use))
                {
                    in = true;
                    break;
                }
            for (auto use : uses)
                if (liveOut.count(use))
                {
                    out = true;
                    break;
                }
            if (in && out)
            {
                begin = std::min(begin, (*(block->begin()))->getNo());
                end = std::max(end, (*(block->rbegin()))->getNo());
            }
            else if (!in && out)
            {
                for (auto i : block->getInsts())
                    if (i->getDef().size() > 0 &&
                        i->getDef()[0] == *(uses.begin()))
                    {
                        begin = std::min(begin, i->getNo());
                        break;
                    }
                end = std::max(end, (*(block->rbegin()))->getNo());
            }
            else if (in && !out)
            {
                begin = std::min(begin, (*(block->begin()))->getNo());
                int temp = 0;
                for (auto use : uses)
                    if (use->getParent()->getParent() == block)
                        temp = std::max(temp, use->getParent()->getNo());
                end = std::max(temp, end);
            }
        }
        interval->start = begin;
        interval->end = end;
    }

    // 合并有重叠使用的活跃区间，确保每个操作数只有一个活跃区间
    /*对所有活跃区间进行两两比较。
    如果两个区间有共同的使用点（即 uses 有交集），则合并这两个区间。
    更新 start 和 end，移除被合并的区间。*/
    bool change;
    change = true;
    while (change)
    {
        change = false;
        std::vector<Interval *> t(intervals.begin(), intervals.end());
        for (size_t i = 0; i < t.size(); i++)
            for (size_t j = i + 1; j < t.size(); j++)
            {
                Interval *w1 = t[i];
                Interval *w2 = t[j];
                if (**w1->defs.begin() == **w2->defs.begin())
                {
                    std::set<MachineOperand *> temp;
                    set_intersection(w1->uses.begin(), w1->uses.end(), w2->uses.begin(), w2->uses.end(), inserter(temp, temp.end()));
                    if (!temp.empty())
                    {
                        change = true;
                        w1->defs.insert(w2->defs.begin(), w2->defs.end());
                        w1->uses.insert(w2->uses.begin(), w2->uses.end());
                        // w1->start = std::min(w1->start, w2->start);
                        // w1->end = std::max(w1->end, w2->end);
                        auto w1Min = std::min(w1->start, w1->end);
                        auto w1Max = std::max(w1->start, w1->end);
                        auto w2Min = std::min(w2->start, w2->end);
                        auto w2Max = std::max(w2->start, w2->end);
                        w1->start = std::min(w1Min, w2Min);
                        w1->end = std::max(w1Max, w2Max);
                        auto it = std::find(intervals.begin(), intervals.end(), w2);
                        if (it != intervals.end())
                            intervals.erase(it);
                    }
                }
            }
    }
    sort(intervals.begin(), intervals.end(), compareStart);
}

// 执行线性扫描寄存器分配，尝试将所有活跃区间映射到物理寄存器，返回是否分配成功，是否需要溢出处理
bool LinearScan::linearScanRegisterAllocation()
{
    // Todo
    /*
        active ←{}
        foreach live interval i, in order of increasing start point
            ExpireOldIntervals(i)
            if length(active) = R then
                SpillAtInterval(i)
            else
                register[i] ← a register removed from pool of free registers
                add i to active, sorted by increasing end point

        活跃区间的扫描：按照起始位置顺序扫描每个活跃区间。
        寄存器分配：为每个活跃区间分配一个物理寄存器，如果没有可用寄存器，则选择一个区间进行溢出。
        活跃列表管理：保持一个 actives 列表，存储当前活跃的区间，并按结束位置排序，以便快速找到需要溢出的区间。
    */
    bool flag = true;

    actives.clear();
    regs.clear();
    for (int i = 4; i <= 10; i++)
        regs.push_back(i);

    for (auto i = intervals.begin(); i != intervals.end(); i++)
    {                           // 按照起始位置顺序扫描每个活跃区间
        expireOldIntervals(*i); // 释放在当前处理的区间（interval）开始之前已经结束的活动区间的寄存器

        if (regs.empty()) // 当前没有可用于分配的物理寄存器
        {
            spillAtInterval(*i); // 选择一个需要溢出的活跃区间，并标记当前区间 i 为溢出
            flag = false;        // 表示分配未完全成功，需要溢出处理
        }
        else // 当前有可用于分配的物理寄存器
        {
            (*i)->rreg = regs.front();                        // 为区间 i 分配物理寄存器
            regs.erase(regs.begin());                         // 从 regs 中移除该寄存器
            actives.push_back((*i));                          // 将区间 i 添加到 actives 中
            sort(actives.begin(), actives.end(), compareEnd); // 按照活跃区间的结束位置对 actives 进行排序，确保最早结束的区间位于前面
        }
    }
    return flag;
}

// 修改机器代码，以反映寄存器分配的结果，将虚拟寄存器替换为分配的物理寄存器
void LinearScan::modifyCode()
{
    for (auto &interval : intervals) // 遍历所有活跃区间
    {
        func->addSavedRegs(interval->rreg); // 记录保存的寄存器
        for (auto def : interval->defs)
            def->setReg(interval->rreg); // 替换定义操作数，将其寄存器编号设置为分配的物理寄存器 rreg
        for (auto use : interval->uses)
            use->setReg(interval->rreg); // 替换使用操作数，将其寄存器编号设置为分配的物理寄存器 rreg
    }
}

// 生成溢出代码（spill code），将需要溢出的虚拟寄存器存储到内存中，并在使用时从内存加载回来
void LinearScan::genSpillCode()
{
    for (auto &interval : intervals)
    {
        if (!interval->spill)
            continue;
        // TODO
        /* HINT:
         * The vreg should be spilled to memory.
         * 1. insert ldr inst before the use of vreg
         * 2. insert str inst after the def of vreg
         */

        interval->disp = -func->AllocSpace(4);

        auto off = new MachineOperand(MachineOperand::IMM, interval->disp);
        auto fp = new MachineOperand(MachineOperand::REG, 11);
        for (auto use : interval->uses)
        {
            auto temp = new MachineOperand(*use);
            MachineOperand *operand = nullptr;
            if (interval->disp > 255 || interval->disp < -255)
            {
                operand = new MachineOperand(MachineOperand::VREG, SymbolTable::getLabel());
                auto inst = new LoadMInstruction(use->getParent()->getParent(), operand, off);
                auto &instructions = use->getParent()->getParent()->getInsts();
                auto it = std::find(instructions.begin(), instructions.end(), use->getParent());
                instructions.insert(it, inst);
                inst = new LoadMInstruction(use->getParent()->getParent(), temp, fp, new MachineOperand(*operand));
                instructions.insert(it, inst);
            }
            else
            {
                auto inst = new LoadMInstruction(use->getParent()->getParent(), temp, fp, off);
                auto &instructions = use->getParent()->getParent()->getInsts();
                auto it = std::find(instructions.begin(), instructions.end(), use->getParent());
                instructions.insert(it, inst);
            }
        }
        for (auto def : interval->defs)
        {
            auto temp = new MachineOperand(*def);
            MachineOperand *operand = nullptr;
            MachineInstruction *inst = nullptr;
            if (interval->disp > 255 || interval->disp < -255)
            {
                operand = new MachineOperand(MachineOperand::VREG, SymbolTable::getLabel());
                inst = new LoadMInstruction(def->getParent()->getParent(), operand, off);
                auto &instructions = def->getParent()->getParent()->getInsts();
                auto it = std::find(instructions.begin(), instructions.end(), def->getParent());
                instructions.insert(++it, inst);
                inst = new StoreMInstruction(def->getParent()->getParent(), temp, fp, new MachineOperand(*operand));
                instructions.insert(++it, inst);
            }
            else
            {
                inst = new StoreMInstruction(def->getParent()->getParent(), temp, fp, off);
                auto &instructions = def->getParent()->getParent()->getInsts();
                auto it = std::find(instructions.begin(), instructions.end(), def->getParent());
                instructions.insert(++it, inst);
            }
        }
    }
}

// 释放那些在当前处理的活跃区间开始之前已经结束的区间所占用的寄存器
void LinearScan::expireOldIntervals(Interval *interval)
{
    // Todo
    /*
        foreach interval j in active, in order of increasing end point
            if endpoint[j] ≥ startpoint[i] then
                return
            remove j from active
            add register[j] to pool of free registers
    */
    // 遍历活跃区间
    for (auto i = actives.begin(); i != actives.end();)
    {
        if ((*i)->end >= interval->start) // 如果当前活动区间 (*i) 的结束时间 end 大于或等于当前区间的开始时间 interval->start，说明该区间还未过期，直接退出循环。
            return;
        // 释放寄存器
        regs.push_back((*i)->rreg);
        sort(regs.begin(), regs.end());                              // 对 regs 进行排序，以便优先使用低编号的寄存器
        i = actives.erase(find(actives.begin(), actives.end(), *i)); // 如果当前区间已过期（结束时间小于当前区间的开始时间），将其占用的寄存器号 (*i)->rreg 回收，存入 regs。
    }
}

// 在当前活跃区间无法分配寄存器时，选择一个区间进行溢出处理
void LinearScan::spillAtInterval(Interval *interval)
{
    // Todo
    /*
        spill ← last interval in active
        if endpoint[spill] > endpoint[i] then
            register[i] ← register[spill]
            location[spill] ← new stack location
            remove spill from active
            add i to active, sorted by increasing end point
        else
            location[i] ← new stack location
    */
    // 获取 actives 列表中的最后一个活跃区间 activeInterval，由于 actives 按结束时间排序，这通常是结束时间最晚的区间
    auto activeInterval = actives.back();
    if (activeInterval->end > interval->end) // 比较结束时间，如果 activeInterval->end > interval->end
    {
        // 说明 activeInterval 的活跃时间比当前区间更长，应优先溢出 activeInterval
        activeInterval->spill = true;                     // 需要溢出
        interval->rreg = activeInterval->rreg;            // 将 activeInterval 的寄存器 rreg 分配给当前区间 interval。
        func->addSavedRegs(interval->rreg);               // 调用 func->addSavedRegs(interval->rreg)，记录需要保存的寄存器。
        actives.push_back(interval);                      // 将当前区间 interval 添加到 actives 中，并重新排序
        sort(actives.begin(), actives.end(), compareEnd); // 如果是 active 列表中的活跃区间结束时间更晚，需要置位其 spill 标志位，并将其占用的寄存器分配给区间 i，再将区间 i 插入到 active 列表中。
    }
    else
    {
        interval->spill = true; // 如果是活跃区间 i 的结束时间更晚，只需要置位其 spill 标志位即可
    }
}

// 提供比较函数，用于排序活跃区间
bool LinearScan::compareStart(Interval *a, Interval *b)
{
    return a->start < b->start;
}
bool LinearScan::compareEnd(Interval *a, Interval *b)
{
    return a->end < b->end;
}
