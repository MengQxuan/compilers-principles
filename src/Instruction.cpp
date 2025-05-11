#include "Instruction.h"
#include "BasicBlock.h"
#include <iostream>
#include "Function.h"
#include "Type.h"
extern FILE *yyout;

// reference
TypeConverInstruction::TypeConverInstruction(Operand *dst, Operand *src, BasicBlock *insert_bb) : Instruction(TYPECONVER, insert_bb), dst(dst), src(src)
{
    dst->setDef(this);
    src->addUse(this);
}

TypeConverInstruction::~TypeConverInstruction()
{
    dst->setDef(nullptr);
    src->addUse(this);
}

void TypeConverInstruction::output() const
{
    // eg. %7 = sitofp i32 %6 to float
    std::string typeConver;
    if (src->getType() == TypeSystem::boolType && dst->getType()->isInt())
    {
        typeConver = "zext";
    }
    else if (src->getType()->isFloat() && dst->getType()->isInt())
    {
        typeConver = "fptosi";
    }
    else if (src->getType()->isInt() && dst->getType()->isFloat())
    {
        typeConver = "sitofp";
    }
    fprintf(yyout, "  %s = %s %s %s to %s\n", dst->toStr().c_str(), typeConver.c_str(), src->getType()->toStr().c_str(), src->toStr().c_str(), dst->getType()->toStr().c_str());
}
Instruction::Instruction(unsigned instType, BasicBlock *insert_bb)
{
    prev = next = this;
    opcode = -1;
    this->instType = instType;
    if (insert_bb != nullptr)
    {
        // fprintf(stderr, "已将指令插入列表\n");
        insert_bb->insertBack(this);
        parent = insert_bb;
    }
}

Instruction::~Instruction()
{
    parent->remove(this);
}

BasicBlock *Instruction::getParent()
{
    return parent;
}

void Instruction::setParent(BasicBlock *bb)
{
    parent = bb;
}

void Instruction::setNext(Instruction *inst)
{
    next = inst;
}

void Instruction::setPrev(Instruction *inst)
{
    prev = inst;
}

Instruction *Instruction::getNext()
{
    return next;
}

Instruction *Instruction::getPrev()
{
    return prev;
}

BinaryInstruction::BinaryInstruction(unsigned opcode, Operand *dst, Operand *src1, Operand *src2, BasicBlock *insert_bb) : Instruction(BINARY, insert_bb)
{
    this->opcode = opcode;
    operands.push_back(dst);
    operands.push_back(src1);
    operands.push_back(src2);
    dst->setDef(this);
    src1->addUse(this);
    src2->addUse(this);
}

BinaryInstruction::~BinaryInstruction()
{
    operands[0]->setDef(nullptr);
    if (operands[0]->usersNum() == 0)
        delete operands[0];
    operands[1]->removeUse(this);
    operands[2]->removeUse(this);
}

void BinaryInstruction::output() const
{
    std::string s1, s2, s3, op, type;
    s1 = operands[0]->toStr();
    s2 = operands[1]->toStr();
    s3 = operands[2]->toStr();
    type = operands[0]->getType()->toStr();
    bool isfloat = false;
    if (operands[1]->getType()->isFloat() || operands[2]->getType()->isFloat())
        isfloat = true;
    if (isfloat && operands[1]->getType()->isInt() && operands[1]->getSymPtr()->isConstant())
    {
        s2 += ".0";
    }
    if (isfloat && operands[2]->getType()->isInt() && operands[2]->getSymPtr()->isConstant())
    {
        s3 += ".0";
    }
    switch (opcode)
    {
    case ADD:
        if (isfloat)
            op = "fadd";
        else
            op = "add";
        break;
    case SUB:
        if (isfloat)
            op = "fsub";
        else
            op = "sub";
        break;
    case MUL:
        if (isfloat)
            op = "fmul";
        else
            op = "mul";
        break;
    case DIV:
        if (isfloat)
            op = "fdiv";
        else
            op = "sdiv";
        break;
    case MOD:
        if (isfloat)
            op = "frem";
        else
            op = "srem";
    default:
        break;
    }
    fprintf(yyout, "  %s = %s %s %s, %s\n", s1.c_str(), op.c_str(), type.c_str(), s2.c_str(), s3.c_str());
}

void XorInstruction::genMachineCode(AsmBuilder *builder)
{
    // TODO 异或运算，主要是在非运算时涉及到的int->bool
    // 与0作比较，相等为1,不等为0（直接进行了非运算）
    auto cur_block = builder->getBlock();
    auto dst = genMachineOperand(operands[0]);
    auto src = genMachineImm(1);
    auto cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, src, MachineInstruction::EQ);
    cur_block->InsertInst(cur_inst);
    src = genMachineImm(0);
    cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, src, MachineInstruction::NE);
    cur_block->InsertInst(cur_inst);
}
XorInstruction::XorInstruction(Operand *dst, Operand *src, BasicBlock *insert_bb) : Instruction(XOR, insert_bb)
{
    operands.push_back(dst);
    operands.push_back(src);
    dst->setDef(this);
    src->addUse(this);
}
void XorInstruction::output() const
{
    std::string dst = operands[0]->toStr();
    std::string src = operands[1]->toStr();
    fprintf(yyout, "  %s = xor i1 %s, true\n", dst.c_str(), src.c_str());
}

UnaryInstruction::~UnaryInstruction()
{
    operands[0]->setDef(nullptr);
    if (operands[0]->usersNum() == 0)
        delete operands[0];
    operands[1]->removeUse(this);
}
void UnaryInstruction::output() const
{

    std::string s1, s2, s3, op, type;
    s1 = operands[0]->toStr();
    s2 = operands[1]->toStr();
    type = operands[0]->getType()->toStr();

    switch (opcode)
    {
    case POS:
        op = "add";
        break;
    case NEG:
        op = "sub";
        break;
    case NOT:
        // if(operands[0]->getType()->isInt()){}
        fprintf(yyout, "  %s = xor %s %s, true\n", s1.c_str(), operands[1]->getType()->toStr().c_str(), s2.c_str());
        return;
    }
    fprintf(yyout, "  %s = %s %s 0, %s\n", s1.c_str(), op.c_str(), operands[1]->getType()->toStr().c_str(), s2.c_str());
}
ZextInstruction::ZextInstruction(Operand *src, Operand *dst, BasicBlock *insert_bb) : Instruction(ZEXT, insert_bb)
{
    operands.push_back(src);
    operands.push_back(dst);
    dst->setDef(this);
    src->addUse(this);
}
void ZextInstruction::output() const
{
    std::string src = operands[0]->toStr();
    std::string dst = operands[1]->toStr();
    std::string src_type = operands[0]->getType()->toStr();
    std::string dst_type = operands[1]->getType()->toStr();
    fprintf(yyout, "  %s = zext %s %s to %s\n", dst.c_str(), src_type.c_str(), src.c_str(), dst_type.c_str());
}
void ZextInstruction::genMachineCode(AsmBuilder *builder)
{
    // TODO 补零指令，实际上用赋值的mov指令即可
    auto cur_block = builder->getBlock();
    auto dst = genMachineOperand(operands[0]);
    auto src = genMachineOperand(operands[1]);
    auto cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, src);
    cur_block->InsertInst(cur_inst);
}

IntFloatCastInstructionn::IntFloatCastInstructionn(unsigned opcode, Operand *src, Operand *dst, BasicBlock *insert_bb) : Instruction(CAST, insert_bb)
{
    this->opcode = opcode;
    operands.push_back(src);
    operands.push_back(dst);
    dst->setDef(this);
    src->addUse(this);
}
CmpInstruction::CmpInstruction(unsigned opcode, Operand *dst, Operand *src1, Operand *src2, BasicBlock *insert_bb) : Instruction(CMP, insert_bb)
{
    this->opcode = opcode;
    operands.push_back(dst);
    operands.push_back(src1);
    operands.push_back(src2);
    dst->setDef(this);
    src1->addUse(this);
    src2->addUse(this);
}

CmpInstruction::~CmpInstruction()
{
    operands[0]->setDef(nullptr);
    if (operands[0]->usersNum() == 0)
        delete operands[0];
    operands[1]->removeUse(this);
    operands[2]->removeUse(this);
}

void CmpInstruction::output() const
{
    std::string s1, s2, s3, op, type, ident;
    s1 = operands[0]->toStr();
    s2 = operands[1]->toStr();
    s3 = operands[2]->toStr();
    type = operands[1]->getType()->toStr();
    if (operands[1]->getType()->isInt())
    {
        ident = "icmp";
        switch (opcode)
        {
        case E:
            op = "eq";
            break;
        case NE:
            op = "ne";
            break;
        case L:
            op = "slt";
            break;
        case LE:
            op = "sle";
            break;
        case G:
            op = "sgt";
            break;
        case GE:
            op = "sge";
            break;
        default:
            op = "";
            break;
        }
    }
    // cout << "cmp" << endl;
    fprintf(yyout, "  %s = %s %s %s %s, %s\n", s1.c_str(), ident.c_str(), op.c_str(), type.c_str(), s2.c_str(), s3.c_str());
}

UncondBrInstruction::UncondBrInstruction(BasicBlock *to, BasicBlock *insert_bb) : Instruction(UNCOND, insert_bb)
{
    branch = to;
}
UnaryInstruction::UnaryInstruction(unsigned opcode, Operand *dst, Operand *src, BasicBlock *insert_bb) : Instruction(BINARY, insert_bb)
{
    this->opcode = opcode;
    operands.push_back(dst);
    operands.push_back(src);
    dst->setDef(this);
    src->addUse(this);
}

void UncondBrInstruction::output() const
{
    fprintf(yyout, "  br label %%B%d\n", branch->getNo());
}

void UncondBrInstruction::setBranch(BasicBlock *bb)
{
    branch = bb;
}

BasicBlock *UncondBrInstruction::getBranch()
{
    return branch;
}

CondBrInstruction::CondBrInstruction(BasicBlock *true_branch, BasicBlock *false_branch, Operand *cond, BasicBlock *insert_bb) : Instruction(COND, insert_bb)
{
    this->true_branch = true_branch;
    this->false_branch = false_branch;
    cond->addUse(this);
    operands.push_back(cond);
}

CondBrInstruction::~CondBrInstruction()
{
    operands[0]->removeUse(this);
}

void CondBrInstruction::output() const
{
    std::string cond, type;
    cond = operands[0]->toStr();
    type = operands[0]->getType()->toStr();
    int true_label = true_branch->getNo();
    int false_label = false_branch->getNo();
    fprintf(yyout, "  br %s %s, label %%B%d, label %%B%d\n", type.c_str(), cond.c_str(), true_label, false_label);
}

void CondBrInstruction::setFalseBranch(BasicBlock *bb)
{
    false_branch = bb;
}

BasicBlock *CondBrInstruction::getFalseBranch()
{
    return false_branch;
}

void CondBrInstruction::setTrueBranch(BasicBlock *bb)
{
    true_branch = bb;
}

BasicBlock *CondBrInstruction::getTrueBranch()
{
    return true_branch;
}

RetInstruction::RetInstruction(Operand *src, BasicBlock *insert_bb) : Instruction(RET, insert_bb)
{
    if (src != nullptr)
    {
        operands.push_back(src);
        src->addUse(this);
    }
}

RetInstruction::~RetInstruction()
{
    if (!operands.empty())
        operands[0]->removeUse(this);
}

void RetInstruction::output() const
{
    fprintf(stderr, "RetInstruction::output()\n");
    if (operands.empty())
    {
        fprintf(yyout, "  ret void\n");
    }
    else
    {
        std::string ret, type;
        ret = operands[0]->toStr();
        type = operands[0]->getType()->toStr();
        fprintf(yyout, "  ret %s %s\n", type.c_str(), ret.c_str());
    }
}

AllocaInstruction::AllocaInstruction(Operand *dst, SymbolEntry *se, BasicBlock *insert_bb) : Instruction(ALLOCA, insert_bb)
{
    operands.push_back(dst);
    dst->setDef(this);
    this->se = se;
}

AllocaInstruction::~AllocaInstruction()
{
    operands[0]->setDef(nullptr);
    if (operands[0]->usersNum() == 0)
        delete operands[0];
}

void AllocaInstruction::output() const
{
    std::string dst, type;
    dst = operands[0]->toStr();
    type = se->getType()->toStr();
    fprintf(yyout, "  %s = alloca %s, align 4\n", dst.c_str(), type.c_str());
}

LoadInstruction::LoadInstruction(Operand *dst, Operand *src_addr, BasicBlock *insert_bb) : Instruction(LOAD, insert_bb)
{
    operands.push_back(dst);
    operands.push_back(src_addr);
    dst->setDef(this);
    src_addr->addUse(this);
    src_addr->addloadUse(this);
}

LoadInstruction::~LoadInstruction()
{
    operands[0]->setDef(nullptr);
    if (operands[0]->usersNum() == 0)
        delete operands[0];
    operands[1]->removeUse(this);
}

void LoadInstruction::output() const
{
    std::string dst = operands[0]->toStr();
    std::string src = operands[1]->toStr();
    std::string src_type;
    std::string dst_type;
    dst_type = operands[0]->getType()->toStr();
    src_type = operands[1]->getType()->toStr();
    fprintf(yyout, "  %s = load %s, %s %s, align 4\n", dst.c_str(), dst_type.c_str(), src_type.c_str(), src.c_str());
}

StoreInstruction::StoreInstruction(Operand *dst_addr, Operand *src, BasicBlock *insert_bb) : Instruction(STORE, insert_bb)
{
    operands.push_back(dst_addr);
    operands.push_back(src);
    dst_addr->addUse(this);
    src->addUse(this);
}

StoreInstruction::~StoreInstruction()
{
    operands[0]->removeUse(this);
    operands[1]->removeUse(this);
}

void StoreInstruction::output() const
{
    std::string dst = operands[0]->toStr();
    std::string src = operands[1]->toStr();
    std::string dst_type = operands[0]->getType()->toStr();
    std::string src_type = operands[1]->getType()->toStr();

    fprintf(yyout, "  store %s %s, %s %s, align 4\n", src_type.c_str(), src.c_str(), dst_type.c_str(), dst.c_str());
}

IntFloatCastInstructionn::~IntFloatCastInstructionn()
{
    operands[0]->setDef(nullptr);
    if (operands[0]->usersNum() == 0)
        delete operands[0];
    operands[1]->removeUse(this);
}

// reference
ToBoolInstruction::ToBoolInstruction(Operand *dst, Operand *src, BasicBlock *insert_bb) : Instruction(TOBOOL, insert_bb), dst(dst), src(src)
{
    src->addUse(this);
    dst->setDef(this);
}

ToBoolInstruction::~ToBoolInstruction()
{
    src->removeUse(this);
    dst->setDef(nullptr);
}

void ToBoolInstruction::output() const
{
    std::string s1, s2, s3, op, type;
    s1 = src->toStr();
    s2 = dst->toStr();
    type = src->getType()->toStr();

    fprintf(yyout, "  %s = icmp ne %s %s, 0\n", s2.c_str(), type.c_str(), s1.c_str());
}
void IntFloatCastInstructionn::output() const
{
    std::string src = operands[0]->toStr();
    std::string dst = operands[1]->toStr();
    std::string src_type = operands[0]->getType()->toStr();
    std::string dst_type = operands[1]->getType()->toStr();
    std::string castType;
    switch (opcode)
    {
    case I2F:
        castType = "sitofp";
        break;
    case F2I:
        castType = "fptosi";
        break;
    default:
        castType = "";
        break;
    }
    fprintf(yyout, "  %s = %s %s %s to %s\n", dst.c_str(), castType.c_str(), src_type.c_str(), src.c_str(), dst_type.c_str());
}

FuncCallInstruction::FuncCallInstruction(SymbolEntry *se, Operand *dst, std::vector<Operand *> params, BasicBlock *insert_bb = nullptr) : Instruction(FUNCTIONCALL, insert_bb), se(se), dst(dst), params(params)
{
    dst->setDef(this);
    for (auto t : params)
    {
        t->addUse(this);
        operands.push_back(t);
    }
}

FuncCallInstruction::~FuncCallInstruction()
{
    dst->setDef(nullptr);
    if (dst->usersNum() == 0)
        delete dst;
    for (auto t : params)
    {
        t->removeUse(this);
    }
}

// reference
GlobalVarDefInstruction::GlobalVarDefInstruction(Operand *dst, ConstantSymbolEntry *se, BasicBlock *insert_bb) : Instruction(GLOBALVAR, insert_bb), dst(dst)
{
    type = ((PointerType *)dst->getType())->getValueType();
    // fprintf(stderr, "创建全局变量定义指令: %s\n", dst->toStr().c_str()); // 输出创建的全局变量定义指令信息

    if (se == nullptr)
    {
        // fprintf(stderr, "全局变量 %s 未初始化\n", dst->toStr().c_str()); // 输出未初始化的全局变量信息

        if (type->isInt())
            value.intValue = 0;
        else if (type->isFloat())
            value.floatValue = 0;
    }
    else
    {
        // fprintf(stderr, "全局变量 %s 初始化为: %s\n", dst->toStr().c_str(), se->toStr().c_str()); // 输出全局变量的初始化值

        if (se->getType()->isInt())
        {
            if (type->isInt())
            {
                value.intValue = se->getValue();
                fprintf(stderr, "整型全局变量 %s 初始化为整数值: %d\n", dst->toStr().c_str(), value.intValue); // 输出整型全局变量的初始化值
            }
            else if (type->isFloat())
            {
                value.floatValue = se->getValue();                                                             // 这里可能有类型转换的问题，应该使用se->getFloat()？
                fprintf(stderr, "浮点型全局变量 %s 初始化为整数值: %d\n", dst->toStr().c_str(), se->getInt()); // 输出浮点型全局变量的初始化值（整型）
            }
        }
        else if (se->getType()->isFloat())
        {
            if (type->isInt())
            {
                value.intValue = se->getValue();                                                                   // 这里可能有类型转换的问题，应该使用se->getInt()？
                fprintf(stderr, "整型全局变量 %s 初始化为浮点数值: %e\n", dst->toStr().c_str(), value.floatValue); // 输出整型全局变量的初始化值（浮点型）
            }
            else if (type->isFloat())
            {
                value.floatValue = se->getValue();
                fprintf(stderr, "浮点型全局变量 %s 初始化为浮点数值: %e\n", dst->toStr().c_str(), value.floatValue); // 输出浮点型全局变量的初始化值
            }
        }
    }
    dst->setDef(this);
}

GlobalVarDefInstruction::~GlobalVarDefInstruction()
{
    // fprintf(stderr, "销毁全局变量定义指令: %s\n", dst->toStr().c_str()); // 输出销毁的全局变量定义指令信息
    dst->setDef(nullptr);
}

void GlobalVarDefInstruction::output() const
{
    std::string ident;
    if (dst->isConst())
        ident = "constant";
    else
        ident = "global";

    if (type->isInt())
    {
        fprintf(yyout, "%s = %s i32 %d, align 4\n", dst->toStr().c_str(), ident.c_str(), value.intValue);
        fprintf(stderr, "输出全局变量定义: %s = %s i32 %d, align 4\n", dst->toStr().c_str(), ident.c_str(), value.intValue); // 输出全局变量定义信息
    }
    else if (type->isFloat())
    {
        fprintf(yyout, "%s = %s float %e, align 4\n", dst->toStr().c_str(), ident.c_str(), value.floatValue);
        fprintf(stderr, "输出全局变量定义: %s = %s float %e, align 4\n", dst->toStr().c_str(), ident.c_str(), value.floatValue); // 输出全局变量定义信息
    }
}

MachineOperand *Instruction::genMachineOperand(Operand *ope)
{
    auto se = ope->getEntry();
    MachineOperand *mope = nullptr;
    if (se->isConstant()) // 常量的话作为立即数
        mope = new MachineOperand(MachineOperand::IMM, dynamic_cast<ConstantSymbolEntry *>(se)->getValue());
    else if (se->isTemporary()) // 临时变量 把其label作为虚拟寄存器创建一个MachineOperand   临时变量%t1会被映射到一个虚拟寄存器
        mope = new MachineOperand(MachineOperand::VREG, dynamic_cast<TemporarySymbolEntry *>(se)->getLabel());
    else if (se->isVariable())
    {
        auto id_se = dynamic_cast<IdentifierSymbolEntry *>(se); // 把se->idse
        if (id_se->isGlobal())
            // 如果是全局变量，将其名称字符串作为内存地址（MEM）创建一个 MachineOperand
            mope = new MachineOperand(id_se->toStr().c_str());
        else
            exit(0);
    }
    return mope;
}

MachineOperand *Instruction::genMachineReg(int reg)
{
    return new MachineOperand(MachineOperand::REG, reg);
}

MachineOperand *Instruction::genMachineVReg()
{
    return new MachineOperand(MachineOperand::VREG, SymbolTable::getLabel());
}

MachineOperand *Instruction::genMachineImm(int val)
{
    return new MachineOperand(MachineOperand::IMM, val);
}

MachineOperand *Instruction::genMachineLabel(int block_no)
{
    std::ostringstream buf;
    buf << ".L" << block_no;
    std::string label = buf.str();
    return new MachineOperand(label);
}

void AllocaInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_func = builder->getFunction();
    int size = se->getType()->getSize() / 8;
    if (size <= 0)
    {
        size = 4;
    }
    int offset = cur_func->AllocSpace(size);
    dynamic_cast<TemporarySymbolEntry *>(operands[0]->getEntry())->setOffset(-offset);
}

void LoadInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_block = builder->getBlock();
    MachineInstruction *cur_inst = nullptr;
    if (operands[1]->getEntry()->isVariable() && dynamic_cast<IdentifierSymbolEntry *>(operands[1]->getEntry())->isGlobal()) // 全局变量
    {
        auto dst = genMachineOperand(operands[0]);
        auto internal_reg1 = genMachineVReg();
        auto internal_reg2 = new MachineOperand(*internal_reg1);
        auto src = genMachineOperand(operands[1]);
        cur_inst = new LoadMInstruction(cur_block, internal_reg1, src);
        cur_block->InsertInst(cur_inst);
        cur_inst = new LoadMInstruction(cur_block, dst, internal_reg2);
        cur_block->InsertInst(cur_inst);
    }
    else if (operands[1]->getEntry()->isTemporary() && operands[1]->getDef() && operands[1]->getDef()->isAlloc()) // 从栈上加载的临时变量
    {
        auto dst = genMachineOperand(operands[0]);
        auto src1 = genMachineReg(11);
        int off = dynamic_cast<TemporarySymbolEntry *>(operands[1]->getEntry())->getOffset();
        auto src2 = genMachineImm(off);
        cur_inst = new LoadMInstruction(cur_block, dst, src1, src2);
        cur_block->InsertInst(cur_inst);
    }
    else
    {
        auto dst = genMachineOperand(operands[0]);
        auto src = genMachineOperand(operands[1]);
        cur_inst = new LoadMInstruction(cur_block, dst, src);
        cur_block->InsertInst(cur_inst);
    }
}

void StoreInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_block = builder->getBlock();
    MachineInstruction *cur_inst = nullptr;
    auto dst = genMachineOperand(operands[0]);
    auto src = genMachineOperand(operands[1]);
    if (operands[1]->getEntry()->isConstant()) // 如果源操作数是常数 先加载到一个虚拟寄存器
    {
        auto dst1 = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, dst1, src);
        cur_block->InsertInst(cur_inst);
        src = new MachineOperand(*dst1); // 更新为新的虚拟寄存器操作数，表示存储操作的数据现在来自于寄存器而非直接的常量
        // 通常不能直接将大于一定范围的立即数存储到内存。因此，需要先将常量加载到寄存器，再将寄存器的值存储到目标地址
    }
    if (operands[0]->getEntry()->isTemporary() && operands[0]->getDef() && operands[0]->getDef()->isAlloc()) // 目的操作数为已分配的临时变量
    {
        auto src1 = genMachineReg(11);                                                        // fp
        int off = dynamic_cast<TemporarySymbolEntry *>(operands[0]->getEntry())->getOffset(); // 获取偏移量
        auto src2 = genMachineImm(off);
        cur_inst = new StoreMInstruction(cur_block, src, src1, src2);
        cur_block->InsertInst(cur_inst);
    }
    else if (operands[0]->getEntry()->isVariable() && dynamic_cast<IdentifierSymbolEntry *>(operands[0]->getEntry())->isGlobal()) // 目的操作数为全局变量
    {
        auto internal_reg1 = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, internal_reg1, dst); // 将全局变量的地址加载到虚拟寄存器
        cur_block->InsertInst(cur_inst);
        cur_inst = new StoreMInstruction(cur_block, src, internal_reg1);
        cur_block->InsertInst(cur_inst);
        /* ldr rX, =global_var
        str rY, [rX] */
    }
    else if (operands[0]->getType()->isPtr()) // 目标操作数为指针
    {
        cur_inst = new StoreMInstruction(cur_block, src, dst);
        cur_block->InsertInst(cur_inst);
    }
}

void BinaryInstruction::genMachineCode(AsmBuilder *builder)
{
    // TODO:
    // complete other instructions
    auto cur_block = builder->getBlock();
    auto dst = genMachineOperand(operands[0]);
    auto src1 = genMachineOperand(operands[1]);
    auto src2 = genMachineOperand(operands[2]);
    /* HINT:
     * The source operands of ADD instruction in ir code both can be immediate num.
     * However, it's not allowed in assembly code.
     * So you need to insert LOAD/MOV instrucrion to load immediate num into register.
     * As to other instructions, such as MUL, CMP, you need to deal with this situation, too.*/
    MachineInstruction *cur_inst = nullptr;
    if (src1->isImm())
    {
        auto internal_reg = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, internal_reg, src1);
        cur_block->InsertInst(cur_inst);
        src1 = new MachineOperand(*internal_reg);
    }
    /*
    合法立即数：
        如果一个立即数小于 0xFF（255）那么直接用前 7～0 位表示即可，此时不用移位，11～8 位的 Rotate_imm 等于 0。
        如果前八位 immed_8 的数值大于 255，那么就看这个数是否能有 immed_8 中的某个数移位 2*Rotate_imm 位形成的。如果能，那么就是合法立即数；否则非法。
    加载不合法立即数：
    只要利用LDR伪指令就可以了，加载任意32位立即数,例如:ldr r1, =12345678
    */
    if (src2->isImm())
    {
        auto internal_reg = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, internal_reg, src2);
        cur_block->InsertInst(cur_inst);
        src2 = new MachineOperand(*internal_reg);
    }
    switch (opcode) /* enum { SUB, ADD, AND, OR, MUL, DIV, MOD };*/
    {
    case ADD:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::ADD, dst, src1, src2);
        break;
    case SUB:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::SUB, dst, src1, src2);
        break;
    case AND:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::AND, dst, src1, src2);
        break;
    case OR:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::OR, dst, src1, src2);
        break;
    case MUL:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::MUL, dst, src1, src2);
        break;
    case DIV:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::DIV, dst, src1, src2);
        break;
    case MOD: /* 不能直接取模，需要经过运算 */
    {
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::DIV, dst, src1, src2); // dst = src1 div src2,
        MachineOperand *tmp = new MachineOperand(*dst);
        src1 = new MachineOperand(*src1);
        src2 = new MachineOperand(*src2);
        cur_block->InsertInst(cur_inst);
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::MUL, tmp, dst, src2); // tmp = dst mul src2
        cur_block->InsertInst(cur_inst);
        dst = new MachineOperand(*tmp);
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::SUB, dst, src1, tmp); // dst = src1 sub tmp
        break;
    }
    default:
        break;
    }
    cur_block->InsertInst(cur_inst);
}

void CmpInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_block = builder->getBlock();
    // 生成操作数1，2
    MachineOperand *src1 = genMachineOperand(operands[1]);
    MachineOperand *src2 = genMachineOperand(operands[2]);
    MachineInstruction *cur_inst = nullptr;
    // load 立即数
    if (src1->isImm()) // 如果 src1 是立即数，需要将其加载到一个虚拟寄存器
    {
        auto internal_reg = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, internal_reg, src1); // 把立即数加载到虚拟寄存器中
        cur_block->InsertInst(cur_inst);
        src1 = new MachineOperand(*internal_reg); // 将 src1 更新为新生成的寄存器操作数
    }
    if (src2->isImm() && ((ConstantSymbolEntry *)(operands[2]->getEntry()))->getValue() > 255) // 如果 src2 是立即数，且大于 255，需要将其加载到一个虚拟寄存器
    {
        auto internal_reg = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, internal_reg, src2);
        cur_block->InsertInst(cur_inst);
        src2 = new MachineOperand(*internal_reg);
    }

    cur_inst = new CmpMInstruction(cur_block, src1, src2, opcode);
    cur_block->InsertInst(cur_inst);
    builder->setCmpOpcode(opcode); // 记录cmp指令的条件码，用于条件分支指令

    // 根据opcode生成相应的条件赋值指令，将比较结果（1或0）存储到目标操作数中
    auto dst = genMachineOperand(operands[0]);
    auto trueOperand = genMachineImm(1);
    auto falseOperand = genMachineImm(0);
    switch (opcode) /*enum {E, NE, L, LE, G, GE}; */
    {
    case E:
    {
        // cout << "eq" << endl;
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, trueOperand, E); // 如果比较结果等于，将1赋值给dst
        cur_block->InsertInst(cur_inst);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, falseOperand, NE); // 如果比较结果不等于，将0赋值给dst
        cur_block->InsertInst(cur_inst);
        break;
    }
    case NE:
    {
        // cout << "ne" << endl;
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, trueOperand, NE);
        cur_block->InsertInst(cur_inst);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, falseOperand, E);
        cur_block->InsertInst(cur_inst);
        break;
    }
    case L:
    {
        // cout << "lt" << endl;
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, trueOperand, L);
        cur_block->InsertInst(cur_inst);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, falseOperand, GE);
        cur_block->InsertInst(cur_inst);
        break;
    }
    case LE:
    {
        // cout << "le" << endl;
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, trueOperand, LE);
        cur_block->InsertInst(cur_inst);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, falseOperand, G);
        cur_block->InsertInst(cur_inst);
        break;
    }
    case G:
    {
        // cout << "gt" << endl;
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, trueOperand, G);
        cur_block->InsertInst(cur_inst);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, falseOperand, LE);
        cur_block->InsertInst(cur_inst);
        break;
    }
    case GE:
    {
        // cout << "ge" << endl;
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, trueOperand, GE);
        cur_block->InsertInst(cur_inst);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, falseOperand, L);
        cur_block->InsertInst(cur_inst);
        break;
    }

    default:
        break;
    }
}

void UncondBrInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_block = builder->getBlock();
    MachineOperand *dst = genMachineLabel(branch->getNo()); // 设置跳转到的目标分支
    auto cur_inst = new BranchMInstruction(cur_block, BranchMInstruction::B, dst);
    cur_block->InsertInst(cur_inst);
}

void CondBrInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_block = builder->getBlock();
    MachineOperand *dst = genMachineLabel(true_branch->getNo()); // 设置跳转到的真分支
    auto cur_inst = new BranchMInstruction(cur_block, BranchMInstruction::B, dst, builder->getCmpOpcode());
    cur_block->InsertInst(cur_inst);

    dst = genMachineLabel(false_branch->getNo()); // 设置跳转到的假分支
    cur_inst = new BranchMInstruction(cur_block, BranchMInstruction::B, dst);
    cur_block->InsertInst(cur_inst);
}

void RetInstruction::genMachineCode(AsmBuilder *builder)
{
    // TODO
    auto cur_block = builder->getBlock();
    if (!operands.empty())
    { // 有返回值 保存返回值到r0
        auto dst = new MachineOperand(MachineOperand::REG, 0);
        auto src = genMachineOperand(operands[0]);
        auto cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, src);
        cur_block->InsertInst(cur_inst);
    }
    // 调整栈帧 保存寄存器sp
    auto sp = new MachineOperand(MachineOperand::REG, 13);
    auto funcSize = new MachineOperand(MachineOperand::IMM, builder->getFunction()->AllocSpace(0)); // 获取函数在堆栈上分配的空间大小
    cur_block->InsertInst(new BinaryMInstruction(cur_block, BinaryMInstruction::ADD, sp, sp, funcSize));
    // bx指令，跳转lr寄存器，实现函数返回
    auto lr = new MachineOperand(MachineOperand::REG, 14);
    cur_block->InsertInst(new BranchMInstruction(cur_block, BranchMInstruction::BX, lr));
}

void UnaryInstruction::genMachineCode(AsmBuilder *builder)
{
    // TODO
    auto cur_block = builder->getBlock();
    MachineOperand *dst = genMachineOperand(operands[0]);
    MachineOperand *src = genMachineOperand(operands[1]);
    MachineInstruction *cur_inst = nullptr;

    switch (opcode)
    {
    case POS:
        // POS 操作：MOV dst, src
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, src);
        break;
    case NEG:
        // NEG 操作：SUB dst, zero, src
        {
            // 生成一个立即数 0
            MachineOperand *zero = genMachineImm(0);
            cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::SUB, dst, zero, src);
        }
        break;
    case NOT:
        // NOT 操作：XOR dst, src, #1
        {
            // 生成一个立即数 1
            // MachineOperand *one = genMachineImm(1);
            // cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::XOR, dst, src, one);
            auto cur_block = builder->getBlock();
            auto dst = genMachineOperand(operands[0]);
            auto src = genMachineImm(1);
            auto cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, src, MachineInstruction::EQ);
            cur_block->InsertInst(cur_inst);
            src = genMachineImm(0);
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, src, MachineInstruction::NE);
            cur_block->InsertInst(cur_inst);
        }
        break;
    default:
        // 处理其他未定义的单目运算符
        fprintf(stderr, "Error: Unknown unary opcode.\n");
        break;
    }

    if (cur_inst)
    {
        cur_block->InsertInst(cur_inst);
    }
}

void FuncCallInstruction::genMachineCode(AsmBuilder *builder)
{
    // TODO
    // 在进行函数调用时，对于含参函数，需要使用 R0-R3 寄存器传递参数，如果参数个数大于四个还需要生成 PUSH 指令来传递参数；
    // 之后生成跳转指令来进入 Callee 函数；
    // 在此之后，需要进行现场恢复的工作，如果之前通过压栈的方式传递了参数，需要恢复 SP 寄存器；
    // 最后，如果函数执行结果被用到，还需要保存 R0 寄存器中的返回值

    auto cur_block = builder->getBlock(); // 获取当前块
    MachineOperand *operand;
    MachineOperand *dst1;
    MachineInstruction *cur_inst;

    // 参数数目  operands.size()=dst+params
    // 如果有返回值，参数数目为 operands.size() - 1
    // 库函数无返回值，我这里直接置为0了,这样写并不完善
    // int paramCount = 0;
    // if (!(dynamic_cast<FunctionType *>(this->se->getType())->getRetType()->isVoid()))
    // {
    //     paramCount = int(operands.size() - 1);
    // }

    // cout << operands.size() << endl;
    int paramCount = operands.size(); // 参数数目=operands.size(),那dst呢？
    if (paramCount <= 4 && paramCount > 0)
    {
        // cout << "param" << endl;
        for (int i = 0; i < paramCount; i++)
        {
            dst1 = genMachineReg(i); // 生成寄存器R0-R3
            operand = genMachineOperand(operands[i]);
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst1, operand); // 将参数值移动到寄存器
            cur_block->InsertInst(cur_inst);
        }
    }

    // // 如果参数个数大于四个还需要生成 PUSH 指令来传递参数；
    // else if (paramCount > 4)
    // {
    //     for (int i = 0; i < 4; i++)
    //     {
    //         dst1 = genMachineReg(i);
    //         operand = genMachineOperand(operands[i]);
    //         // 判断是否是立即数
    //         if (operand->isImm())
    //         {
    //             cur_inst = new LoadMInstruction(cur_block, dst1, operand);
    //         }
    //         else
    //         {
    //             // cout << "ssss";
    //             cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst1, operand);
    //         }
    //         cur_block->InsertInst(cur_inst);
    //     }
    //     for (int j = 4; j < paramCount; j++)
    //     {
    //         operand = genMachineOperand(operands[j]);
    //         if (operand->isImm())
    //         {
    //             cur_inst = new LoadMInstruction(cur_block, genMachineVReg(), operand);
    //             cur_block->InsertInst(cur_inst);
    //             operand = genMachineVReg();
    //         }
    //         std::vector<MachineOperand *> temp; //
    //         cur_inst = new StackMInstrcuton(cur_block, StackMInstrcuton::PUSH, temp, operand);
    //         cur_block->InsertInst(cur_inst);
    //     }
    // }

    // 之后生成跳转指令来进入 Callee 函数；
    cur_inst = new BranchMInstruction(cur_block, BranchMInstruction::BL, new MachineOperand(se->toStr().c_str()));
    cur_block->InsertInst(cur_inst);

    // 如果之前通过压栈的方式传递了参数，需要恢复 SP 寄存器；
    // if (paramCount > 4)
    // {
    //     MachineOperand *sp = new MachineOperand(MachineOperand::REG, 13);
    //     cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::ADD, sp, sp, genMachineImm((operands.size() - 4) * 4));
    //     cur_block->InsertInst(cur_inst);
    // }

    // 返回值不为空，还需要保存 R0 寄存器中的返回值
    if (!(dynamic_cast<FunctionType *>(this->se->getType())->getRetType()->isVoid()))
    {
        // cout << "r0";
        MachineOperand *r0 = new MachineOperand(MachineOperand::REG, 0);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, genMachineOperand(dst), r0); // 把R0移动到dst
        cur_block->InsertInst(cur_inst);
    }
}

void ToBoolInstruction::genMachineCode(AsmBuilder *builder)
{
    // TODO
}

void TypeConverInstruction::genMachineCode(AsmBuilder *builder)
{
    // TODO
}

void GlobalVarDefInstruction::genMachineCode(AsmBuilder *builder)
{
    // TODO
}
