#ifndef __MACHINECODE_H__
#define __MACHINECODE_H__
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <fstream>
#include "SymbolTable.h"

/* Hint:
 * MachineUnit: Compiler unit
 * MachineFunction: Function in assembly code
 * MachineInstruction: Single assembly instruction
 * MachineOperand: Operand in assembly instruction, such as immediate number, register, address label */

/* Todo:
 * We only give the example code of "class BinaryMInstruction" and "class AccessMInstruction" (because we believe in you !!!),
 * You need to complete other the member function, especially "output()" ,
 * After that, you can use "output()" to print assembly code . */

class MachineUnit;
class MachineFunction;
class MachineBlock;
class MachineInstruction;

class MachineOperand
{
private:
    MachineInstruction *parent; // 指向父指令的指针，表示该操作数所属的指令
    int type;
    int val;           // value of immediate number 立即数的值
    int reg_no;        // register no 寄存器编号
    std::string label; // address label 地址标签
public:
    enum
    {
        IMM,
        VREG,
        REG,
        LABEL
    };
    MachineOperand(int tp, int val);
    MachineOperand(std::string label);
    bool operator==(const MachineOperand &) const;
    bool operator<(const MachineOperand &) const;
    bool isImm() { return this->type == IMM; };
    bool isReg() { return this->type == REG; };
    bool isVReg() { return this->type == VREG; };
    bool isLabel() { return this->type == LABEL; };
    int getVal() { return this->val; };
    int getReg() { return this->reg_no; };
    void setReg(int regno)
    {
        this->type = REG;
        this->reg_no = regno;
    };
    std::string getLabel() { return this->label; };
    void setParent(MachineInstruction *p) { this->parent = p; };
    MachineInstruction *getParent() { return this->parent; };
    void PrintReg();
    void output();
};

class MachineInstruction
{
protected:
    MachineBlock *parent;
    int no;                              // 指令编号
    int type;                            // Instruction type 指令类型
    int cond = MachineInstruction::NONE; // Instruction execution condition, optional !!  指令的执行条件
    int op;                              // Instruction opcode 指令操作码
    // Instruction operand list, sorted by appearance order in assembly instruction
    std::vector<MachineOperand *> def_list;
    std::vector<MachineOperand *> use_list;
    void addDef(MachineOperand *ope) { def_list.push_back(ope); };
    void addUse(MachineOperand *ope) { use_list.push_back(ope); };
    // Print execution code after printing opcode
    void PrintCond();
    enum instType
    {
        BINARY,
        LOAD,
        STORE,
        MOV,
        BRANCH,
        CMP,
        STACK
    };

public:
    enum condType
    {
        EQ,
        NE,
        LT,
        LE,
        GT,
        GE,
        NONE
    };
    virtual void output() = 0;
    void setNo(int no) { this->no = no; };
    int getNo() { return no; };
    std::vector<MachineOperand *> &getDef() { return def_list; };
    std::vector<MachineOperand *> &getUse() { return use_list; };
    MachineBlock *getParent() { return this->parent; };
};

class BinaryMInstruction : public MachineInstruction
{
public:
    enum opType
    {
        ADD,
        SUB,
        MUL,
        DIV,
        AND,
        OR,
        XOR
        // XOR
    }; // 单目在这里写???
    BinaryMInstruction(MachineBlock *p, int op,
                       MachineOperand *dst, MachineOperand *src1, MachineOperand *src2,
                       int cond = MachineInstruction::NONE);
    void output();
};

class LoadMInstruction : public MachineInstruction
{
public:
    LoadMInstruction(MachineBlock *p,
                     MachineOperand *dst, MachineOperand *src1, MachineOperand *src2 = nullptr,
                     int cond = MachineInstruction::NONE);
    void output();
};

class StoreMInstruction : public MachineInstruction
{
public:
    StoreMInstruction(MachineBlock *p,
                      MachineOperand *src1, MachineOperand *src2, MachineOperand *src3 = nullptr,
                      int cond = MachineInstruction::NONE);
    void output();
};

class MovMInstruction : public MachineInstruction
{
public:
    enum opType
    {
        MOV,
        MVN // MVN 表示按位取反移动操作，将一个操作数（源 src）的内容按位取反后移动到另一个操作数（目的 dst）没用到吧
    };
    MovMInstruction(MachineBlock *p, int op,
                    MachineOperand *dst, MachineOperand *src,
                    int cond = MachineInstruction::NONE);
    void output();
};

class BranchMInstruction : public MachineInstruction
{
public:
    enum opType
    {
        B,  // 无条件分支
        BL, // 链接分支
        BX  // 寄存器分支
    };
    BranchMInstruction(MachineBlock *p, int op,
                       MachineOperand *dst,
                       int cond = MachineInstruction::NONE);
    void output();
};

class CmpMInstruction : public MachineInstruction
{
public:
    enum opType
    {
        CMP
    };
    CmpMInstruction(MachineBlock *p,
                    MachineOperand *src1, MachineOperand *src2,
                    int cond = MachineInstruction::NONE);
    void output();
};

class StackMInstrcuton : public MachineInstruction
{
public:
    enum opType
    {
        PUSH,
        POP
    };
    StackMInstrcuton(MachineBlock *p, int op,
                     std::vector<MachineOperand *> regs,
                     MachineOperand *src1,
                     MachineOperand *src2 = nullptr,
                     int cond = MachineInstruction::NONE);
    void output();
};

class MachineBlock
{
private:
    MachineFunction *parent;
    int no;
    int cmpCond;                            // 比较条件
    std::vector<MachineBlock *> pred, succ; // 存储前驱和后继基本块的指针列表，用于构建控制流图（CFG）
    std::vector<MachineInstruction *> inst_list;
    // 存储基本块中活动寄存器的集合，live_in表示进入该基本块时活动的寄存器，live_out表示离开该基本块时活动的寄存器。
    std::set<MachineOperand *> live_in;
    std::set<MachineOperand *> live_out;

public:
    std::vector<MachineInstruction *> &getInsts() { return inst_list; };
    // 正向迭代器，用于遍历指令列表。
    std::vector<MachineInstruction *>::iterator begin() { return inst_list.begin(); };
    std::vector<MachineInstruction *>::iterator end() { return inst_list.end(); };

    // 反向迭代器，用于反向遍历指令列表
    std::vector<MachineInstruction *>::reverse_iterator rbegin() { return inst_list.rbegin(); };
    std::vector<MachineInstruction *>::reverse_iterator rend() { return inst_list.rend(); };

    MachineBlock(MachineFunction *p, int no)
    {
        this->parent = p;
        this->no = no;
    };
    void InsertInst(MachineInstruction *inst) { this->inst_list.push_back(inst); };
    void addPred(MachineBlock *p) { this->pred.push_back(p); }; // 添加前驱基本块
    void addSucc(MachineBlock *s) { this->succ.push_back(s); }; // 添加后继基本块

    // 返回该基本块的live_in和live_out集合的引用
    std::set<MachineOperand *> &getLiveIn() { return live_in; };
    std::set<MachineOperand *> &getLiveOut() { return live_out; };

    // 返回该基本块的前驱和后继列表的引用
    std::vector<MachineBlock *> &getPreds() { return pred; };
    std::vector<MachineBlock *> &getSuccs() { return succ; };
    int getSize() const { return inst_list.size(); }; // 该基本块中指令的数量
    void output();
    int getCmpCond() const { return cmpCond; }; // 获取比较条件，条件分支用
    MachineFunction *getParent() { return parent; }
};

class MachineFunction
{
private:
    MachineUnit *parent;
    std::vector<MachineBlock *> block_list;
    int stack_size;           // 函数使用的栈空间大小，用于管理局部变量
    std::set<int> saved_regs; // 该函数需要保存的寄存器编号集。这些寄存器在函数调用前被保存，在函数返回后恢复。
    SymbolEntry *sym_ptr;

public:
    std::vector<MachineBlock *> &getBlocks() { return block_list; };              // 返回该函数中基本块列表的引用，用于访问和操作这些基本块
    std::vector<MachineBlock *>::iterator begin() { return block_list.begin(); }; // 返回指向基本块列表起始位置的迭代器，用于遍历基本块
    std::vector<MachineBlock *>::iterator end() { return block_list.end(); };     // 返回指向基本块列表结束位置的迭代器，用于遍历基本块
    MachineFunction(MachineUnit *p, SymbolEntry *sym_ptr);
    /* HINT:
     * Alloc stack space for local variable;
     * return current frame offset ;
     * we store offset in symbol entry of this variable in function AllocInstruction::genMachineCode()
     * you can use this function in LinearScan::genSpillCode() */

    // 用于为局部变量分配栈空间，并返回当前帧偏移量。这个偏移量随后被存储在符号表条目中，以便在后续的代码生成过程中引用。
    int AllocSpace(int size)
    {
        this->stack_size += size;
        return this->stack_size;
    };
    void InsertBlock(MachineBlock *block) { this->block_list.push_back(block); }; // 将一个基本块插入到函数的基本块列表中
    void addSavedRegs(int regno) { saved_regs.insert(regno); };                   // 将一个寄存器编号添加到需要保存的寄存器集合中
    void output();
    std::vector<MachineOperand *> getSavedRegs(); // 返回保存的寄存器操作数列表，用于在函数调用前后保存和恢复寄存器状态
    MachineUnit *getParent() const { return parent; };
};

class MachineUnit
{
private:
    std::vector<MachineFunction *> func_list;
    std::vector<SymbolEntry *> global_list;

public:
    std::vector<MachineFunction *> &getFuncs() { return func_list; };
    std::vector<MachineFunction *>::iterator begin() { return func_list.begin(); };
    std::vector<MachineFunction *>::iterator end() { return func_list.end(); };
    void InsertFunc(MachineFunction *func) { func_list.push_back(func); };
    void InsertGlobal(SymbolEntry *global) { global_list.push_back(global); };
    void output();
    void PrintGlobalDecl();
    void PrintGlobal();
};

#endif