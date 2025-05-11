#include "MachineCode.h"
#include <cstring>
#include <iostream>
#include "Instruction.h"

extern FILE *yyout;

MachineOperand::MachineOperand(int tp, int val)
{
    this->type = tp;
    if (tp == MachineOperand::IMM)
        this->val = val;
    else
        this->reg_no = val;
}

MachineOperand::MachineOperand(std::string label)
{
    this->type = MachineOperand::LABEL;
    this->label = label;
}

bool MachineOperand::operator==(const MachineOperand &a) const
{
    if (this->type != a.type)
        return false;
    if (this->type == IMM)
        return this->val == a.val;
    return this->reg_no == a.reg_no;
}

bool MachineOperand::operator<(const MachineOperand &a) const
{
    if (this->type == a.type)
    {
        if (this->type == IMM)
            return this->val < a.val;
        return this->reg_no < a.reg_no;
    }
    return this->type < a.type;

    if (this->type != a.type)
        return false;
    if (this->type == IMM)
        return this->val == a.val;
    return this->reg_no == a.reg_no;
}

void MachineOperand::PrintReg()
{
    switch (reg_no)
    {
    case 11:
        fprintf(yyout, "fp");
        break;
    case 13:
        fprintf(yyout, "sp");
        break;
    case 14:
        fprintf(yyout, "lr");
        break;
    case 15:
        fprintf(yyout, "pc");
        break;
    default:
        fprintf(yyout, "r%d", reg_no);
        break;
    }
}

void MachineOperand::output()
{
    /* HINT：print operand
     * Example:
     * immediate num 1 -> print #1;
     * register 1 -> print r1;
     * lable addr_a -> print addr_a; */
    switch (this->type)
    {
    case IMM:
        fprintf(yyout, "#%d", this->val);
        break;
    case VREG:
        fprintf(yyout, "v%d", this->reg_no);
        break;
    case REG:
        PrintReg();
        break;
    case LABEL:
        if (this->label.substr(0, 2) == ".L")
            fprintf(yyout, "%s", this->label.c_str());
        else if (!this->label.empty() && this->label[0] == '@')
            fprintf(yyout, "addr_%s", this->label.c_str() + 1); // 去掉第一个字符 '@'
        else
            fprintf(yyout, "addr_%s", this->label.c_str());
    default:
        break;
    }
}

void MachineInstruction::PrintCond() // 打印指令执行条件
{
    // TODO
    switch (cond)
    {
    case EQ:
        fprintf(yyout, "eq");
        break;
    case NE:
        fprintf(yyout, "ne");
        break;
    case LT:
        fprintf(yyout, "lt");
        break;
    case LE:
        fprintf(yyout, "le");
        break;
    case GT:
        fprintf(yyout, "gt");
        break;
    case GE:
        fprintf(yyout, "ge");
        break;
    default:
        break;
    }
}

BinaryMInstruction::BinaryMInstruction(
    MachineBlock *p, int op,
    MachineOperand *dst, MachineOperand *src1, MachineOperand *src2,
    int cond)
{
    this->parent = p;
    this->type = MachineInstruction::BINARY;
    this->op = op;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src1);
    this->use_list.push_back(src2);
    dst->setParent(this);
    src1->setParent(this);
    src2->setParent(this);
}

void BinaryMInstruction::output()
{
    // TODO:
    // Complete other instructions
    switch (this->op)
    {
    case BinaryMInstruction::ADD:
        fprintf(yyout, "\tadd ");
        break;
    case BinaryMInstruction::SUB:
        fprintf(yyout, "\tsub ");
        break;
    case BinaryMInstruction::AND:
        fprintf(yyout, "\tand ");
        break;
    case BinaryMInstruction::OR:
        fprintf(yyout, "\torr ");
        break;
    case BinaryMInstruction::MUL:
        fprintf(yyout, "\tmul ");
        break;
    case BinaryMInstruction::DIV:
        fprintf(yyout, "\tsdiv ");
        break;
    case BinaryMInstruction::XOR:
        fprintf(yyout, "\teor ");
        break;
    default:
        break;
    }
    this->PrintCond();
    this->def_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[1]->output();
    fprintf(yyout, "\n");
}

LoadMInstruction::LoadMInstruction(MachineBlock *p,
                                   MachineOperand *dst, MachineOperand *src1, MachineOperand *src2,
                                   int cond)
{
    this->parent = p;
    this->type = MachineInstruction::LOAD;
    this->op = -1;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src1);
    if (src2) // 如果存在偏移量
        this->use_list.push_back(src2);
    dst->setParent(this);
    src1->setParent(this);
    if (src2)
        src2->setParent(this);
}

void LoadMInstruction::output()
{
    fprintf(yyout, "\tldr ");
    this->def_list[0]->output();
    fprintf(yyout, ", ");

    // Load immediate num, eg: ldr r1, =8
    if (this->use_list[0]->isImm())
    {
        fprintf(yyout, "=%d\n", this->use_list[0]->getVal());
        return;
    }

    // Load address
    if (this->use_list[0]->isReg() || this->use_list[0]->isVReg())
        fprintf(yyout, "[");

    this->use_list[0]->output();
    if (this->use_list.size() > 1)
    {
        fprintf(yyout, ", ");
        this->use_list[1]->output();
    }

    if (this->use_list[0]->isReg() || this->use_list[0]->isVReg())
        fprintf(yyout, "]");
    fprintf(yyout, "\n");
}

StoreMInstruction::StoreMInstruction(MachineBlock *p,
                                     MachineOperand *src1, MachineOperand *src2, MachineOperand *src3,
                                     int cond)
{
    this->parent = p;                       // 当前指令所属的基本块
    this->type = MachineInstruction::STORE; // 指令类型设为 STORE
    this->op = -1;                          // 操作码，-1 表示未设置具体操作
    this->cond = cond;                      // 条件码（如 EQ、NE 等）
    this->use_list.push_back(src1);         // 添加源操作数 src1（存储值）
    this->use_list.push_back(src2);         // 添加源操作数 src2（存储地址）
    if (src3)                               // 如果有偏移量 src3，则加入
        this->use_list.push_back(src3);
    src1->setParent(this); // 设置 src1 的父指针为当前指令
    src2->setParent(this); // 设置 src2 的父指针为当前指令
    if (src3)
        src3->setParent(this); // 设置 src3 的父指针为当前指令（如果有）
}

void StoreMInstruction::output()
{
    fprintf(yyout, "\tstr ");    // 输出存储指令 "str"
    this->use_list[0]->output(); // 输出存储的数据
    fprintf(yyout, ", ");        // 添加逗号分隔符
    // 输出存储地址
    if (this->use_list[1]->isReg() || this->use_list[1]->isVReg())
        fprintf(yyout, "[");
    this->use_list[1]->output(); // 输出基地址
    if (this->use_list.size() > 2)
    { // 如果有偏移量
        fprintf(yyout, ", ");
        this->use_list[2]->output(); // 输出偏移量
    }
    if (this->use_list[1]->isReg() || this->use_list[1]->isVReg())
        fprintf(yyout, "]");
    fprintf(yyout, "\n");
}

MovMInstruction::MovMInstruction(MachineBlock *p, int op,
                                 MachineOperand *dst, MachineOperand *src,
                                 int cond)
{
    // TODO 设置一些成员变量的值
    this->parent = p;
    this->op = op; // 指令设置操作码，这里操作码用于指示具体的移动操作类型
    this->cond = cond;
    this->type = MachineInstruction::MOV;
    this->addDef(dst);
    this->addUse(src);
    // 设置两个操作数的parent，以便操作数能够跟踪到其所属的指令
    dst->setParent(this);
    src->setParent(this);
}

void MovMInstruction::output()
{
    fprintf(yyout, "\tmov");
    PrintCond(); // 打印条件码
    fprintf(yyout, " ");
    // cout << def_list[0]->getLabel() << endl;
    this->def_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[0]->output();
    fprintf(yyout, "\n");
}

BranchMInstruction::BranchMInstruction(MachineBlock *p, int op,
                                       MachineOperand *dst,
                                       int cond)
{
    this->parent = p;
    this->op = op;
    this->cond = cond;
    this->type = MachineInstruction::BRANCH;
    this->addUse(dst);
    dst->setParent(this);
}

void BranchMInstruction::output()
{
    // TODO
    // 这里把BL单独处理，解决库函数调用的前缀addr_问题
    if (op == BL)
    {
        fprintf(yyout, "\tbl ");
        if (!this->use_list[0]->getLabel().empty() && this->use_list[0]->getLabel()[0] == '@')
        {
            fprintf(yyout, "%s", this->use_list[0]->getLabel().c_str() + 1);
        }
        fprintf(yyout, "\n");
    }
    else
    {
        switch (op)
        {
        case B:
            fprintf(yyout, "\tb");
            break;
        case BX:
        {
            // 输出一个 POP 指令来恢复帧指针（fp）和链接寄存器（lr）
            auto fp = new MachineOperand(MachineOperand::REG, 11);
            auto lr = new MachineOperand(MachineOperand::REG, 14);
            auto cur_inst = new StackMInstrcuton(this->getParent(), StackMInstrcuton::POP, this->getParent()->getParent()->getSavedRegs(), fp, lr);
            cur_inst->output();
            fprintf(yyout, "\tbx"); // 跳转到链接寄存器（lr）中存储的地址，即函数的调用地址
            break;
        }
        default:
            break;
        }
        PrintCond();
        fprintf(yyout, " ");
        this->use_list[0]->output();
        fprintf(yyout, "\n");
    }
}

CmpMInstruction::CmpMInstruction(MachineBlock *p,
                                 MachineOperand *src1, MachineOperand *src2,
                                 int cond)
{
    // TODO
    this->parent = p;
    this->type = MachineInstruction::CMP;
    this->op = -1;
    this->cond = cond;
    this->use_list.push_back(src1);
    this->use_list.push_back(src2);
    src1->setParent(this);
    src2->setParent(this);
}

void CmpMInstruction::output()
{
    // TODO
    // Jsut for reg alloca test
    // delete it after test
    fprintf(yyout, "\tcmp ");
    this->use_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[1]->output();
    fprintf(yyout, "\n");
}

StackMInstrcuton::StackMInstrcuton(MachineBlock *p, int op, std::vector<MachineOperand *> srcs, MachineOperand *src, MachineOperand *src1, int cond)
{
    this->parent = p;
    this->op = op;
    this->cond = cond;
    this->type = MachineInstruction::STACK;
    if (srcs.size() != 0)
    {
        for (auto reg : srcs) // 遍历 srcs 中的每一个源操作数 reg
        {
            this->addUse(reg); // 将源操作数 reg 添加到当前指令的使用操作数列表中
        }
    }
    if (src != nullptr)
    {
        this->use_list.push_back(src); // 将源操作数 src 添加到当前指令的使用操作数列表 use_list 中
        src->setParent(this);          // 设置源操作数 src 的父指令为当前指令 this。
        // 建立了操作数与指令之间的双向关联，便于在后续的指令生成过程中追踪操作数的使用情况
    }
    if (src1 != nullptr)
    {
        this->use_list.push_back(src1);
        src1->setParent(this);
    }
}

void StackMInstrcuton::output()
{

    switch (op)
    {
    case PUSH:
        fprintf(yyout, "\tpush ");
        break;
    case POP:
        fprintf(yyout, "\tpop ");
        break;
    }
    fprintf(yyout, "{");
    this->use_list[0]->output();
    for (long unsigned int i = 1; i < use_list.size(); i++)
    {
        fprintf(yyout, ", ");
        this->use_list[i]->output();
    }
    fprintf(yyout, "}\n");
}

MachineFunction::MachineFunction(MachineUnit *p, SymbolEntry *sym_ptr)
{
    this->parent = p;
    this->sym_ptr = sym_ptr;
    this->stack_size = 0;
};

void MachineBlock::output() // 输出该基本块的所有指令
{
    fprintf(yyout, ".L%d:\n", this->no);
    for (auto iter : inst_list)
        iter->output();
}

void MachineFunction::output()
{
    // std::cout<<"111";
    // 分配足够的内存来存储函数名
    char *func_name = new char[strlen(this->sym_ptr->toStr().c_str()) + 1]; // 加1是为了存放'\0'
    strcpy(func_name, this->sym_ptr->toStr().c_str() + 1);                  // 将函数名从符号表条目中复制到新分配的内存中，并且跳过了第一个字符@
    fprintf(yyout, "\t.global %s\n", func_name);
    fprintf(yyout, "\t.type %s , %%function\n", func_name);
    fprintf(yyout, "%s:\n", func_name); // 输出函数的标签，表示函数的开始位置
    // TODO 生成函数前导代码
    /* Hint:
     *  1. Save fp  保存帧指针（fp）
     *  2. fp = sp  设置帧指针为栈指针（sp）
     *  3. Save callee saved register   保存被调用者保存的寄存器（callee saved register）
     *  4. Allocate stack space for local variable  为局部变量分配栈空间*/

    // Traverse all the block in block_list to print assembly code.

    MachineOperand *fp = new MachineOperand(MachineOperand::REG, 11);
    MachineOperand *sp = new MachineOperand(MachineOperand::REG, 13);
    MachineOperand *lr = new MachineOperand(MachineOperand::REG, 14);
    MachineInstruction *cur_inst;

    cur_inst = new StackMInstrcuton(nullptr, StackMInstrcuton::PUSH, this->getSavedRegs(), fp, lr);
    cur_inst->output();

    cur_inst = new MovMInstruction(nullptr, MovMInstruction::MOV, fp, sp);
    cur_inst->output();

    cur_inst = new BinaryMInstruction(nullptr, BinaryMInstruction::SUB, sp, sp, new MachineOperand(MachineOperand::IMM, AllocSpace(0)));
    cur_inst->output();

    for (auto iter : block_list)
        iter->output();
    fprintf(yyout, "\n");
}

std::vector<MachineOperand *> MachineFunction::getSavedRegs() // 返回保存的寄存器操作数列表，用于在函数调用前后保存和恢复寄存器状态
{
    std::vector<MachineOperand *> regs;
    for (auto i : saved_regs)
    {
        MachineOperand *reg = new MachineOperand(MachineOperand::REG, i);
        regs.push_back(reg);
    }
    return regs;
}

void MachineUnit::PrintGlobalDecl()
{
    // 判断是否有全局变量或常量
    if (!global_list.empty())
    {
        fprintf(yyout, "\t.data\n");
    }
    std::vector<IdentifierSymbolEntry *> Global_list;

    for (auto global : global_list)
    {
        // cout<<"a";
        IdentifierSymbolEntry *se = (IdentifierSymbolEntry *)global;
        // 获取变量名并去除开头的 '@'（如果存在）
        std::string varName = se->toStr();
        if (!varName.empty() && varName[0] == '@')
        {
            // cout << "@@@@" << endl;
            // cout << "1" << varName << endl;
            varName = varName.substr(1);
            // cout << "2" << varName << endl;
        }
        // 如果是常量，将其加入 Global_list
        if (se->isConstant())
        {
            Global_list.push_back(se);
        }
        else
        {
            // 输出普通全局变量的信息
            fprintf(yyout, ".global %s\n", varName.c_str());
            fprintf(yyout, "\t.size %s, %d\n", varName.c_str(), se->getType()->getSize() / 8);
            fprintf(yyout, "%s:\n", varName.c_str());

            // 输出变量值
            fprintf(yyout, "\t.word %d\n", se->getValue());
        }
    }

    // 如果有常量，进入只读数据段
    if (!Global_list.empty())
    {
        fprintf(yyout, ".section .rodata\n");

        for (auto con : Global_list)
        {
            IdentifierSymbolEntry *se = con;
            // 获取变量名并去除开头的 '@'（如果存在）
            std::string varName = se->toStr();
            if (!varName.empty() && varName[0] == '@')
            {
                // cout << "@@@@" << endl;
                // cout << "1" << varName << endl;
                varName = varName.substr(1);
                // cout << "2" << varName << endl;
            }
            fprintf(yyout, ".global %s\n", varName.c_str());
            fprintf(yyout, "\t.size %s, %d\n", varName.c_str(), se->getType()->getSize() / 8);
            fprintf(yyout, "%s:\n", varName.c_str());

            // 输出常量值
            fprintf(yyout, "\t.word %d\n", se->getValue());
        }
    }
}

void MachineUnit::PrintGlobal()
{
    for (auto s : global_list)
    {
        IdentifierSymbolEntry *se = (IdentifierSymbolEntry *)s;
        // 获取变量名并去除开头的 '@'（如果存在）
        std::string varName = se->toStr();
        if (!varName.empty() && varName[0] == '@')
        {
            // cout << "@@@@" << endl;
            // cout << "1" << varName << endl;
            varName = varName.substr(1);
            // cout << "2" << varName << endl;
        }
        fprintf(yyout, "addr_%s:\n", varName.c_str());
        fprintf(yyout, "\t.word %s\n", varName.c_str());
    }
}
void MachineUnit::output()
{
    fprintf(yyout, "\t.arch armv8-a\n");
    fprintf(yyout, "\t.arch_extension crc\n");
    fprintf(yyout, "\t.arm\n");
    PrintGlobalDecl();
    fprintf(yyout, "\t.text\n");
    for (auto iter : func_list)
        iter->output();
    PrintGlobal();
}
