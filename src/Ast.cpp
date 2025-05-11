#include "Ast.h"
#include "SymbolTable.h"
#include "Unit.h"
#include "Instruction.h"
#include "IRBuilder.h"
#include <string>
#include "Type.h"
using namespace std;

extern FILE *yyout;
int Node::counter = 0;
extern MachineUnit mUnit;
IRBuilder *Node::builder = nullptr;
Type *returnType = nullptr;
bool funcReturned = false;
Type *retType = nullptr;
bool hasRet = false;

Node::Node()
{
    seq = counter++;
    next = nullptr;
}

void Node::backPatch(std::vector<Instruction *> &list, BasicBlock *bb)
{
    for (auto &inst : list)
    {
        if (inst->isCond())
            dynamic_cast<CondBrInstruction *>(inst)->setTrueBranch(bb);
        else if (inst->isUncond())
            dynamic_cast<UncondBrInstruction *>(inst)->setBranch(bb);
    }
}

std::vector<Instruction *> Node::merge(std::vector<Instruction *> &list1, std::vector<Instruction *> &list2)
{
    std::vector<Instruction *> res(list1);
    res.insert(res.end(), list2.begin(), list2.end()); // 把list2的元素添加到res的末尾
    return res;
}

void Ast::genCode(Unit *unit)
{
    // cout << "Ast::genCode(Unit *unit)" << endl;
    IRBuilder *builder = new IRBuilder(unit);
    Node::setIRBuilder(builder);
    root->genCode();
}

void FunctionDef::genCode()
{
    // cout << "FunctionDef::genCode()" << endl;
    Unit *unit = builder->getUnit();         // 获取当前正在生成的Unit
    Function *func = new Function(unit, se); // 创建一个新的函数
    BasicBlock *entry = func->getEntry();    // 获取函数的入口块
    // set the insert point to the entry basicblock of this function.
    builder->setInsertBB(entry);    // 设置当前插入点为函数的入口块
    DeclStmt *curr = FuncDefParams; // 获取函数定义的参数列表
    while (curr != nullptr)         // 遍历函数定义的参数列表
    {
        func->addParam(curr->getId()->getSymPtr()); // 为函数添加参数
        curr->genCode();
        curr = (DeclStmt *)curr->getNext();
    }
    stmt->genCode();

    /**
     * Construct control flow graph. You need do set successors and predecessors for each basic block.
     * Todo
     */
    // 建立块之间的关系
    for (auto bb : *func)
    {
        Instruction *curr = bb->begin(); // 获取当前基本块的第一个指令
        bool isDelete = false;
        for (; curr != bb->end();) // 遍历当前基本块的指令
        {
            if (isDelete) // 如果当前指令已经被删除，则跳过
            {
                Instruction *next = curr->getNext();
                bb->remove(curr);
                delete curr;
                curr = next;
                continue;
            }
            if (curr->isCond() && curr != bb->rbegin()) // 如果当前指令是条件跳转且不是最后一条指令，则设置它的后继和前驱
            {
                Instruction *next = curr->getNext(); // 获取当前指令的后继
                bb->remove(curr);                    // 获取当前指令的前后指令
                delete curr;
                curr = next;
                continue;
            }
            if (curr->isRet() || curr->isUncond()) // 如果当前指令是返回指令或无条件跳转
            {
                isDelete = true; // 将当前指令标记为删除
            }
            curr = curr->getNext(); // 指向下一条指令
        }
        Instruction *last = bb->rbegin(); // 获取当前基本块的最后一条指令
        if (last->isUncond())             // 如果最后一条指令是无条件跳转
        {
            UncondBrInstruction *ucbr = dynamic_cast<UncondBrInstruction *>(last); // 转换为无条件跳转指令
            BasicBlock *bb_branch = ucbr->getBranch();                             // 获取无条件跳转的目标基本块
            bb->addSucc(bb_branch);                                                // 该基本块的后继是ucbr指向的基本块
            bb_branch->addPred(bb);                                                // ucbr指向的基本块的前驱是该基本块
        }
        else if (last->isCond()) // 如果最后一条指令是条件跳转
        {
            CondBrInstruction *cbr = dynamic_cast<CondBrInstruction *>(last);   // 转换为条件跳转指令
            BasicBlock *tb = cbr->getTrueBranch(), *fb = cbr->getFalseBranch(); // 获取条件跳转的两个目标基本块
            bb->addSucc(tb);
            bb->addSucc(fb);
            tb->addPred(bb);
            fb->addPred(bb);
        }
        else if (!last->isRet())
        {
            new RetInstruction(nullptr, bb); // 最后一条指令不是返回指令，则添加一个返回指令到该基本块
        }
    }
}

int BinaryExpr::getValue()
{
    int value = 0;
    switch (op)
    {
    case ADD:
        value = expr1->getValue() + expr2->getValue();
        break;
    case SUB:
        value = expr1->getValue() - expr2->getValue();
        break;
    case MUL:
        value = expr1->getValue() * expr2->getValue();
        break;
    case DIV:
        if (expr2->getValue())
            value = expr1->getValue() / expr2->getValue();
        break;
    case MOD:
        if (expr2->getValue())
        {
            value = (int)(expr1->getValue()) % (int)(expr2->getValue());
        }
        break;
    case AND:
        value = expr1->getValue() && expr2->getValue();
        break;
    case OR:
        value = expr1->getValue() || expr2->getValue();
        break;
    case LESS:
        value = expr1->getValue() < expr2->getValue();
        break;
    case LESSEQUAL:
        value = expr1->getValue() <= expr2->getValue();
        break;
    case GREATER:
        value = expr1->getValue() > expr2->getValue();
        break;
    case GREATEREQUAL:
        value = expr1->getValue() >= expr2->getValue();
        break;
    case EQUAL:
        value = expr1->getValue() == expr2->getValue();
        break;
    case NOTEQUAL:
        value = expr1->getValue() != expr2->getValue();
        break;
    }
    return value;
}

void BinaryExpr::genCode()
{
    BasicBlock *bb = builder->getInsertBB(); // 获取当前基本块
    Function *func = bb->getParent();        // 获取当前基本块所属的函数

    if (op == AND)
    {
        BasicBlock *trueBB = new BasicBlock(func); // if the result of lhs is true, jump to the trueBB.
        expr1->genCode();
        if (expr1->getOperand()->getType() != TypeSystem::boolType)
        {
            expr1->toBool(func);
        }
        backPatch(expr1->trueList(), trueBB); // 如果expr1为真，跳转到trueBB
        builder->setInsertBB(trueBB);         // set the insert point to the trueBB so that intructions generated by expr2 will be inserted into it.
        expr2->genCode();
        if (expr2->getOperand()->getType() != TypeSystem::boolType)
        {
            expr2->toBool(func);
        }
        true_list = expr2->trueList();                              // expr2 的 trueList 即为整个表达式的 trueList
        false_list = merge(expr1->falseList(), expr2->falseList()); // 合并 falseList
    }
    else if (op == OR)
    {
        BasicBlock *falseBB = new BasicBlock(func);
        expr1->genCode();
        if (expr1->getOperand()->getType() != TypeSystem::boolType)
        {
            fprintf(stderr, "expr1的类型= %s\n", expr1->getOperand()->getType()->toStr().c_str());
            expr1->toBool(func);
        }
        backPatch(expr1->falseList(), falseBB);
        builder->setInsertBB(falseBB); // set the insert point to the trueBB so that intructions generated by expr2 will be inserted into it.
        expr2->genCode();
        if (expr2->getOperand()->getType() != TypeSystem::boolType)
        {
            expr2->toBool(func);
        }
        false_list = expr2->falseList();
        true_list = merge(expr1->trueList(), expr2->trueList());
    }
    else if (op >= LESS && op <= NOTEQUAL)
    {
        expr1->genCode();
        expr2->genCode();

        Operand *src1 = expr1->getOperand();
        Operand *src2 = expr2->getOperand();

        int opcode;
        switch (op)
        {
        case LESS:
            opcode = CmpInstruction::L;
            break;
        case LESSEQUAL:
            opcode = CmpInstruction::LE;
            break;
        case GREATER:
            opcode = CmpInstruction::G;
            break;
        case GREATEREQUAL:
            opcode = CmpInstruction::GE;
            break;
        case EQUAL:
            opcode = CmpInstruction::E;
            break;
        case NOTEQUAL:
            opcode = CmpInstruction::NE;
            break;
        default:
            opcode = -1;
            break;
        }

        if (src1->getType() != src2->getType())
        {
            if (src1->getType() != TypeSystem::boolType && src2->getType() == TypeSystem::boolType)
            {
                Operand *srcNew = new Operand(new TemporarySymbolEntry(src1->getType(), SymbolTable::getLabel()));
                new TypeConverInstruction(srcNew, src2, bb);
                src2 = srcNew;
            }
            else
            // (src2->getType() != TypeSystem::boolType && src1->getType() == TypeSystem::boolType)
            {
                Operand *srcNew = new Operand(new TemporarySymbolEntry(src2->getType(), SymbolTable::getLabel()));
                new TypeConverInstruction(srcNew, src1, bb);
                src1 = srcNew;
            }
        }
        dst = new Operand(new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel()));
        new CmpInstruction(opcode, dst, src1, src2, bb);

        BasicBlock *trueBB = new BasicBlock(func);
        BasicBlock *falseBB = new BasicBlock(func);
        BasicBlock *tempBB = new BasicBlock(func);

        this->true_list.push_back(new CondBrInstruction(trueBB, tempBB, dst, bb));
        this->false_list.push_back(new UncondBrInstruction(falseBB, tempBB));
    }
    else if (op >= ADD && op <= MOD)
    {
        expr1->genCode();
        expr2->genCode();

        Operand *src1 = expr1->getOperand();
        Operand *src2 = expr2->getOperand();

        int opcode;
        switch (op)
        {
        case ADD:
            opcode = BinaryInstruction::ADD;
            break;
        case SUB:
            opcode = BinaryInstruction::SUB;
            break;
        case MUL:
            opcode = BinaryInstruction::MUL;
            break;
        case DIV:
            opcode = BinaryInstruction::DIV;
            break;
        case MOD:
            opcode = BinaryInstruction::MOD;
            break;
        default:
            opcode = -1;
            break;
        }

        if (dst->getType() != src1->getType())
        {
            Operand *srcNew = new Operand(new TemporarySymbolEntry(dst->getType(), SymbolTable::getLabel()));
            new TypeConverInstruction(srcNew, src1, bb);
            src1 = srcNew;
        }
        if (dst->getType() != src2->getType())
        {
            Operand *srcNew = new Operand(new TemporarySymbolEntry(dst->getType(), SymbolTable::getLabel()));
            new TypeConverInstruction(srcNew, src2, bb);
            src2 = srcNew;
        }

        new BinaryInstruction(opcode, dst, src1, src2, bb);
    }
}

// void BinaryExpr::genCode()
// {
//     cout << "BinaryExpr::genCode()" << endl;
//     BasicBlock *bb = builder->getInsertBB();
//     Function *func = bb->getParent();
//     cout << "1111111111111111111" << endl;
//     if (op == AND)
//     {
//         BasicBlock *trueBB = new BasicBlock(func); // if the result of lhs is true, jump to the trueBB.
//         expr1->genCode();
//         if (expr1->getOperand()->getType() != TypeSystem::boolType)
//             expr1->toBool(func);
//         backPatch(expr1->trueList(), trueBB);
//         builder->setInsertBB(trueBB); // set the insert point to the trueBB so that intructions generated by expr2 will be inserted into it.
//         expr2->genCode();
//         if (expr2->getOperand()->getType() != TypeSystem::boolType)
//             expr2->toBool(func);
//         true_list = expr2->trueList();
//         false_list = merge(expr1->falseList(), expr2->falseList());
//     }
//     else if (op == OR)
//     {
//         // Todo
//         BasicBlock *falseBB = new BasicBlock(func);
//         expr1->genCode();
//         if (expr1->getOperand()->getType() != TypeSystem::boolType)
//             expr1->toBool(func);
//         backPatch(expr1->falseList(), falseBB);
//         builder->setInsertBB(falseBB); // set the insert point to the trueBB so that intructions generated by expr2 will be inserted into it.
//         expr2->genCode();
//         if (expr2->getOperand()->getType() != TypeSystem::boolType)
//             expr2->toBool(func);
//         false_list = expr2->falseList();
//         true_list = merge(expr1->trueList(), expr2->trueList());
//     }
//     else if (op >= LESS && op <= NOTEQUAL)
//     {
//         // Todo
//         cout<<"比较运算符中间代码生成"<<endl;
//         expr1->genCode();
//         expr2->genCode();
//         Operand *src1 = expr1->getOperand();
//         Operand *src2 = expr2->getOperand();
//         int opcode;
//         switch (op)
//         {
//         case LESS:
//             opcode = CmpInstruction::L;
//             break;
//         case LESSEQUAL:
//             opcode = CmpInstruction::LE;
//             break;
//         case GREATER:
//             opcode = CmpInstruction::G;
//             break;
//         case GREATEREQUAL:
//             opcode = CmpInstruction::GE;
//             break;
//         case EQUAL:
//             opcode = CmpInstruction::E;
//             break;
//         case NOTEQUAL:
//             opcode = CmpInstruction::NE;
//             break;
//         default:
//             opcode = -1;
//             break;
//         }
//         if (src1->getType() != src2->getType())
//         {
//             if (src1->getType() != TypeSystem::boolType && src2->getType() == TypeSystem::boolType)
//             {
//                 Operand *srcNew = new Operand(new TemporarySymbolEntry(src1->getType(), SymbolTable::getLabel()));
//                 new TypeConverInstruction(srcNew, src2, bb);
//                 src2 = srcNew;
//             }
//             else if (src2->getType() != TypeSystem::boolType && src1->getType() == TypeSystem::boolType)
//             {
//                 Operand *srcNew = new Operand(new TemporarySymbolEntry(src2->getType(), SymbolTable::getLabel()));
//                 new TypeConverInstruction(srcNew, src1, bb);
//                 src1 = srcNew;
//             }
//             if(src1->getType()->isFloat() && expr2->getSymPtr()->isConstant())
//             {

//             }
//         }
//         new CmpInstruction(opcode, dst, src1, src2, bb);
//         BasicBlock *trueBB = new BasicBlock(func);
//         BasicBlock *falseBB = new BasicBlock(func);
//         BasicBlock *tempBB = new BasicBlock(func);
//         UncondBrInstruction* uncondInst = new UncondBrInstruction(falseBB, tempBB);
// CondBrInstruction* condInst = new CondBrInstruction(trueBB, tempBB, dst, bb);

// // 将指令对象指针转换为 BasicBlock** 存储在 true_list 和 false_list 中
// BasicBlock** trueInstWrapper = reinterpret_cast<BasicBlock**>(&condInst);
// BasicBlock** falseInstWrapper = reinterpret_cast<BasicBlock**>(&uncondInst);

// // 将包装后的指针推入列表
// this->true_list.push_back(trueInstWrapper);
// this->false_list.push_back(falseInstWrapper);
//     }
//     else if (op >= ADD && op <= MOD)
//     {
//         expr1->genCode();
//         expr2->genCode();
//         Operand *src1 = expr1->getOperand();
//         Operand *src2 = expr2->getOperand();
//         int opcode;
//         switch (op)
//         {
//         case ADD:
//             opcode = BinaryInstruction::ADD;
//             break;
//         case SUB:
//             opcode = BinaryInstruction::SUB;
//             break;
//         case MUL:
//             opcode = BinaryInstruction::MUL;
//             break;
//         case DIV:
//             opcode = BinaryInstruction::DIV;
//             break;
//         case MOD:
//             opcode = BinaryInstruction::MOD;
//             break;
//         default:
//             opcode = -1;
//             break;
//         }
//         if (dst->getType() != src1->getType())
//         {
//             Operand *srcNew = new Operand(new TemporarySymbolEntry(dst->getType(), SymbolTable::getLabel()));
//             new TypeConverInstruction(srcNew, src1, bb);
//             src1 = srcNew;
//         }
//         if (dst->getType() != src2->getType())
//         {
//             Operand *srcNew = new Operand(new TemporarySymbolEntry(dst->getType(), SymbolTable::getLabel()));
//             new TypeConverInstruction(srcNew, src2, bb);
//             src2 = srcNew;
//         }
//         new BinaryInstruction(opcode, dst, src1, src2, bb);
//     }
// }
int ConstExpr::getValue()
{
    return ((ConstantSymbolEntry *)symbolEntry)->getValue();
}
void Constant::genCode()
{
    // we don't need to generate code.
}
int Constant::getValue()
{
    return ((ConstantSymbolEntry *)symbolEntry)->getValue();
}
int Id::getValue()
{
    return ((IdentifierSymbolEntry *)symbolEntry)->getValue();
}
void Id::genCode()
{
    // cout << "Id::genCode()" << endl;

    BasicBlock *bb = builder->getInsertBB();
    Operand *addr = dynamic_cast<IdentifierSymbolEntry *>(symbolEntry)->getAddr();
    new LoadInstruction(dst, addr, bb);
}

void IfStmt::genCode()
{
    Function *func;
    BasicBlock *then_bb, *end_bb;

    func = builder->getInsertBB()->getParent();
    then_bb = new BasicBlock(func);
    end_bb = new BasicBlock(func);

    cond->genCode();
    if (cond->getOperand()->getType() != TypeSystem::boolType)
        cond->toBool(func);
    backPatch(cond->trueList(), then_bb);
    backPatch(cond->falseList(), end_bb);

    builder->setInsertBB(then_bb);
    thenStmt->genCode();
    then_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, then_bb);

    builder->setInsertBB(end_bb);
}

void IfElseStmt::genCode()
{
    // Todo
    // cout << "IfElseStmt::genCode()" << endl;
    Function *func;
    BasicBlock *then_bb, *else_bb, *end_bb;

    func = builder->getInsertBB()->getParent();
    then_bb = new BasicBlock(func);
    else_bb = new BasicBlock(func);
    end_bb = new BasicBlock(func);
    fprintf(stderr, "ifelsestmt中cond= %d\n", cond->cbcaivalue);
    cond->genCode();
    fprintf(stderr, "cond->getOperand()->getType()->toStr() = %s\n", cond->getOperand()->getType()->toStr().c_str());

    if (cond->getOperand()->getType() != TypeSystem::boolType)
    {
        cond->toBool(func);
    }

    backPatch(cond->trueList(), then_bb);
    backPatch(cond->falseList(), else_bb);

    builder->setInsertBB(then_bb);
    thenStmt->genCode();
    then_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, then_bb);

    builder->setInsertBB(else_bb);
    elseStmt->genCode();
    else_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, else_bb);

    builder->setInsertBB(end_bb);
}

void CompoundStmt::genCode()
{
    // cout << "CompoundStmt::genCode()" << endl;
    if (stmt)
        stmt->genCode();
}

void SeqNode::genCode()
{
    // Todo
    // cout << "SeqNode::genCode()" << endl;

    for (auto p : stmts)
    {
        p->genCode();
    }
}

// reference
void ExprNode::toBool(Function *func)
{
    TemporarySymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, identifiers->getLabel());
    Operand *tmp = new Operand(se);

    fprintf(stderr, "Operand Type: %s\n", tmp->getType()->toStr().c_str());
    BasicBlock *bb = builder->getInsertBB();

    BasicBlock *trueBB = new BasicBlock(func);
    BasicBlock *falseBB = new BasicBlock(func);
    BasicBlock *tempBB = new BasicBlock(func);

    new ToBoolInstruction(tmp, dst, bb);

    this->true_list.push_back(new CondBrInstruction(trueBB, tempBB, tmp, bb));
    this->false_list.push_back(new UncondBrInstruction(falseBB, tempBB));
    this->dst = tmp;
    // cout << "Operand Tyspe: " << dst->getType()->toStr() << endl;
}

// void ExprNode::toBool(Function *func)
// {
//     TemporarySymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, identifiers->getLabel());
//     Operand *tmp = new Operand(se);
//     BasicBlock *bb = builder->getInsertBB();
//     BasicBlock *trueBB = new BasicBlock(func);
//     BasicBlock *falseBB = new BasicBlock(func);
//     BasicBlock *tempBB = new BasicBlock(func);
//     new ToBoolInstruction(tmp, dst, bb);
//     UncondBrInstruction* uncondInst = new UncondBrInstruction(falseBB, tempBB);
//     CondBrInstruction* condInst = new CondBrInstruction(trueBB, tempBB, dst, bb);

//     // 将指令对象指针转换为 BasicBlock** 存储在 true_list 和 false_list 中
//     BasicBlock** trueInstWrapper = reinterpret_cast<BasicBlock**>(&condInst);
//     BasicBlock** falseInstWrapper = reinterpret_cast<BasicBlock**>(&uncondInst);

// // 将包装后的指针推入列表
//     this->true_list.push_back(trueInstWrapper);
//     this->false_list.push_back(falseInstWrapper);
//     this->dst = tmp;
// }
void DeclStmt::genCode()
{

    // IdentifierSymbolEntry *se = dynamic_cast<IdentifierSymbolEntry *>(id->getSymPtr());
    // if (se->isGlobal())
    // {
    //     Operand *addr;
    //     SymbolEntry *addr_se;
    //     addr_se = new IdentifierSymbolEntry(*se);
    //     addr_se->setType(new PointerType(se->getType()));
    //     addr = new Operand(addr_se);
    //     se->setAddr(addr);
    // }
    // else if (se->isLocal())
    // {
    //     Function *func = builder->getInsertBB()->getParent();
    //     BasicBlock *entry = func->getEntry();
    //     Instruction *alloca;
    //     Operand *addr;
    //     SymbolEntry *addr_se;
    //     Type *type;
    //     type = new PointerType(se->getType());
    //     addr_se = new TemporarySymbolEntry(type, SymbolTable::getLabel());
    //     addr = new Operand(addr_se);
    //     alloca = new AllocaInstruction(addr, se); // allocate space for local id in function stack.
    //     entry->insertFront(alloca);               // allocate instructions should be inserted into the begin of the entry block.
    //     se->setAddr(addr);                        // set the addr operand in symbol entry so that we can use it in subsequent code generation.
    // }

    if (next != nullptr)
    {
        next->genCode(); // 递归生成下一个声明语句
    }

    IdentifierSymbolEntry *se = dynamic_cast<IdentifierSymbolEntry *>(id->getSymPtr());
    // fstderr, "处理声明: %s\n", se->toStr().c_str()); // 输出正在处理的声明的名称

    if (se->isGlobal())
    {
        // fprintf(stderr, "全局变量声明: %s\n", se->toStr().c_str()); // 输出全局变量声明信息
        Operand *addr;
        SymbolEntry *addr_se;
        addr_se = new IdentifierSymbolEntry(*se);
        addr_se->setType(new PointerType(se->getType())); // 为全局变量生成指针类型
        addr = new Operand(addr_se);
        se->setAddr(addr);
        mUnit.InsertGlobal(se);
        if (expr == nullptr)
        {
            // fprintf(stderr, "全局变量 %s 未初始化\n", se->toStr().c_str()); // 输出未初始化的全局变量信息
            builder->getUnit()->insertGlobalVar(new GlobalVarDefInstruction(addr, nullptr, builder->getInsertBB()));
        }
        else
        {
            ConstantSymbolEntry *valueEntry = (ConstantSymbolEntry *)expr->getSymPtr();
            int val = valueEntry->getValue();
            // cout << "全局变量 %s 初始化为" << val; // 输出全局变量的初始化值
            ((IdentifierSymbolEntry *)se)->setValue(val);
            builder->getUnit()->insertGlobalVar(new GlobalVarDefInstruction(addr, valueEntry, builder->getInsertBB()));
        }
    }
    else if (se->isLocal())
    {
        // fprintf(stderr, "局部变量声明: %s\n", se->toStr().c_str()); // 输出局部变量声明信息

        Function *func = builder->getInsertBB()->getParent();
        BasicBlock *entry = func->getEntry();
        Instruction *alloca;
        Operand *addr;
        SymbolEntry *addr_se;
        Type *type;
        type = new PointerType(se->getType());
        addr_se = new TemporarySymbolEntry(type, SymbolTable::getLabel());
        addr = new Operand(addr_se);

        alloca = new AllocaInstruction(addr, se); // allocate space for local id in function stack.
        // 确保分配指令插入到入口基本块的前部，保证函数执行前局部变量已经分配好空间
        entry->insertFront(alloca); // allocate instructions should be inserted into the begin of the entry block.
        se->setAddr(addr);          // set the addr operand in symbol entry so that we can use it in subsequent code generation.

        if (expr != nullptr)
        {
            if (!isParam)
            {
                expr->genCode();
                new StoreInstruction(addr, expr->getOperand(), builder->getInsertBB());
                // fprintf(stderr, "局部变量 %s 赋值为表达式结果\n", se->toStr().c_str()); // 输出局部变量赋值信息
            }
            else
            {
                new StoreInstruction(addr, ((IdentifierSymbolEntry *)expr->getSymPtr())->getAddr(), builder->getInsertBB());
                // fprintf(stderr, "局部变量 %s 作为参数赋值\n", se->toStr().c_str()); // 输出局部变量作为参数的赋值信息
            }
        }
    }
    else if (se->isParam())
    {
        // fprintf(stderr, "参数声明: %s\n", se->toStr().c_str()); // 输出参数声明信息

        Operand *addr;
        SymbolEntry *addr_se;
        addr_se = new TemporarySymbolEntry(se->getType(), SymbolTable::getLabel());
        addr = new Operand(addr_se);
        se->setAddr(addr);
    }
}

void ReturnStmt::genCode()
{
    // cout << "returnStmt::genCode()" << endl;
    //  Todo
    BasicBlock *bb = builder->getInsertBB();
    Operand *op = nullptr;
    if (retValue != nullptr)
    {
        retValue->genCode();
        op = retValue->getOperand();
    }
    new RetInstruction(op, bb);
}
void WhileStmt::genCode()
{
    Function *func;
    BasicBlock *while_bb, *then_bb, *end_bb;

    func = builder->getInsertBB()->getParent();
    while_bb = new BasicBlock(func);
    then_bb = new BasicBlock(func);
    end_bb = new BasicBlock(func);

    new UncondBrInstruction(while_bb, builder->getInsertBB());
    builder->setInsertBB(while_bb);
    cond->genCode();
    if (cond->getOperand()->getType() != TypeSystem::boolType)
        cond->toBool(func);
    backPatch(cond->trueList(), then_bb); // 条件为真，回填到then_bb
    backPatch(cond->falseList(), end_bb); // 条件为假，回填到end_bb

    while_bb = builder->getInsertBB();
    builder->setInsertBB(then_bb);
    stmt->genCode();
    backPatch(builder->getWhileList(true), while_bb); // 回填到while_bb
    backPatch(builder->getWhileList(false), end_bb);  // 回填到end_bb
    then_bb = builder->getInsertBB();

    new UncondBrInstruction(while_bb, then_bb); // 在尾部添加无条件跳转到while_bb

    builder->setInsertBB(end_bb);
}

void BreakStmt::genCode()
{
}

void ContinueStmt::genCode()
{
}

void AssignStmt::genCode()
{
    // cout << "AssignStmt::genCode()" << endl;
    BasicBlock *bb = builder->getInsertBB();
    expr->genCode();
    Operand *addr = dynamic_cast<IdentifierSymbolEntry *>(lval->getSymPtr())->getAddr();
    Operand *src = expr->getOperand();
    /***
     * We haven't implemented array yet, the lval can only be ID. So we just store the result of the `expr` to the addr of the id.
     * If you want to implement array, you have to caculate the address first and then store the result into it.
     */
    if (((PointerType *)addr->getType())->getValueType() != src->getType())
    {
        Operand *srcNew = new Operand(new TemporarySymbolEntry(addr->getType(), SymbolTable::getLabel()));
        new TypeConverInstruction(srcNew, src, bb);
        src = srcNew;
    }
    new StoreInstruction(addr, src, bb);
}

void Ast::typeCheck()
{
    SymbolEntry *se = identifiers->lookup("main");
    if (se == nullptr)
    {
        fprintf(stderr, "类型检查错误：没有找到main函数\n");
        exit(EXIT_FAILURE); // 退出编译器
    }
    if (root != nullptr)
    {
        root->typeCheck();
    }
}

void FunctionDef::typeCheck()
{
    returnType = ((FunctionType *)se->getType())->getRetType();
    string name = se->toStr();
    // cout << name;
    string type = se->getType()->toStr();
    // cout << type;
    if (name == "@main" && type != "i32()")
    {
        fprintf(stderr, "函数返回值错误\n");
        exit(EXIT_FAILURE);
    }
    stmt->typeCheck();
    //     if(returnType->isInt())cout<<"asdsda";

    //        stmt->typeCheck();
    //             exit(EXIT_FAILURE);}
    // cout<<"FunctionDef::typeCheck()"<<endl;
    //     // 输出函数名称和类型信息，帮助调试
    //     if (se != nullptr) {

    //     } else {
    //         cout << "SymbolEntry (se) is nullptr!" << endl;
    //     }

    //     // 获取函数的返回值类型

    // 判断函数是否返回
    // funcReturned = false;

    //     // 调用stmt->typeCheck()时，输出正在检查的语句
    //     if (stmt != nullptr) {
    //         fprintf(stderr,"准备检查函数定义的stmt\n");
    //         stmt->typeCheck();
    //     } else {
    //         cout << "Function body (stmt) is nullptr!" << endl;
    //     }

    if (!funcReturned && !returnType->isVoid())
    {
        fprintf(stderr, "类型检查错误：int类型函数无返回值或返回值为void\n");
        exit(EXIT_FAILURE);
    }

    // 如果是void类型函数且没有写return，需要补上一个隐式的return
    if (!funcReturned && returnType->isVoid())
    {
        this->voidAddRet = new ReturnStmt(nullptr);
    }

    // if (se->toStr() == "main") {
    //         cout << "正在检查 main 函数的返回类型..." << endl;
    //         if (!returnType->isInt()) {
    //             fprintf(stderr, "类型检查错误：main 函数必须返回 int 类型\n");
    //             exit(EXIT_FAILURE);
    //         }
    //     }
    //     returnType = nullptr;
}

void EmptyStmt::typeCheck()
{
}
void EmptyStmt::genCode()
{
}
void ExprStmt::typeCheck()
{
    expr->typeCheck();
}
void ExprStmt::genCode()
{
    expr->genCode();
}
void FuncCallExp::typeCheck()
{
    // fprintf(stderr,"已进入FuncCallExp::typeCheck()\n");
    FunctionType *t = (FunctionType *)funcSym->getType();
    ExprNode *curr = param;
    int i = 0;
    while (curr != nullptr)
    {
        // fprintf(stderr,"此时param是%s\n",param->getSymbolEntry()->getType()->toStr().c_str());
        curr->typeCheck();
        Type *p = t->getParamsType(i);
        if (p == nullptr)
        {
            fprintf(stderr, "类型检查错误：函数参数数量错误\n");
            exit(EXIT_FAILURE);
        }
        // fprintf(stderr,"是否为空指针 %d\n",p==nullptr);
        // fprintf(stderr,"此时p是%s\n",p->toStr().c_str());

        if (p->isVoid())
        {
            fprintf(stderr, "类型检查错误：函数参数类型错误\n");
            exit(EXIT_FAILURE);
        }
        Type *currType = curr->getSymPtr()->getType();
        if (currType->isVoid())
        {
            fprintf(stderr, "类型检查错误：函数参数类型错误\n");
            exit(EXIT_FAILURE);
        }
        if (currType->isFunc())
            currType = ((FunctionType *)currType)->getRetType();
        if (!Type::isValid(p, currType))
        {
            fprintf(stderr, "类型检查错误：函数参数类型错误\n");
            exit(EXIT_FAILURE);
        }

        i++;
        curr = (ExprNode *)curr->getNext();
    }
    if (t->getParamsType(i) != nullptr)
    {
        fprintf(stderr, "类型检查错误：函数参数数量错误\n");
        exit(EXIT_FAILURE);
    }
}
void FuncCallExp::genCode()
{
    // cout << "FuncCallExp::genCode()" << endl;
    std::vector<Operand *> params;
    ExprNode *curr = param;
    BasicBlock *bb = builder->getInsertBB();
    while (curr != nullptr)
    {
        curr->genCode();
        params.push_back(curr->getOperand());
        curr = (ExprNode *)curr->getNext();
    }
    new FuncCallInstruction(funcSym, dst, params, bb);
    string type = symbolEntry->getType()->toStr();
}
void ExprNode::typeCheck()
{
}
void ExprNode::genCode()
{
}
int UnaryExpr::getValue()
{
    int value = 0;
    switch (op)
    {
    case NOT:
        value = !(expr->getValue());
        break;
    case NEG:
        value = -(expr->getValue());
        break;
    }
    return value;
}

void UnaryExpr::genCode()
{
    // Todo
    BasicBlock* bb = builder->getInsertBB();
    if (op == NOT) 
    {
        expr->genCode();
        Operand* src = expr->getOperand();
        if (!expr->getOperand()->getType()->isBool()) 
        {//int->bool 与零作比较
            Operand* temp = new Operand(new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel()));
            new CmpInstruction(CmpInstruction::NE, temp, src, new Operand(new ConstantSymbolEntry(TypeSystem::intType, 0)), bb);
            src = temp;
        }
        new XorInstruction(dst, src, bb);

        Operand* temp1 = new Operand(new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel()));
        new CmpInstruction(CmpInstruction::NE, temp1, src, new Operand(new ConstantSymbolEntry(TypeSystem::intType, 1)), bb);
        BasicBlock* bb = builder->getInsertBB();
        Function* func = bb->getParent();
        BasicBlock *truebb = new BasicBlock(func);
        BasicBlock *falsebb = new BasicBlock(func);
        BasicBlock *tempbb = new BasicBlock(func);
        Instruction* temp = new CondBrInstruction(truebb,tempbb,dst,bb);
        this->trueList().push_back(temp);
        temp=new UncondBrInstruction(falsebb, tempbb);
        this->falseList().push_back(temp);
    } 
    else if (op == NEG || op == POS) 
    {
        expr->genCode();
        Operand* src2 = expr->getOperand();//获得表达式源操作数

        // to complete
        if (src2->getType() == TypeSystem::boolType) 
        {
            Operand* temp =new Operand(new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel()));
            new ZextInstruction(temp, expr->dst,bb); //bool型扩展为整形
            expr->dst = temp;   
            src2 = temp; 
        }
        int opcode=0;
        switch (op)
        {
        case POS:
            opcode = BinaryInstruction::ADD;
            break;
        case NEG:
            opcode = BinaryInstruction::SUB;
            break;
        default:
            break;
        }
        Operand* src1 = new Operand(new ConstantSymbolEntry(TypeSystem::intType, 0));
        new BinaryInstruction(opcode, dst, src1, src2, bb);
    }
}

void BinaryExpr::typeCheck()
{

    // cout << "BinaryExpr::typeCheck()" << endl;
    expr1->typeCheck();
    expr2->typeCheck();

    if (op == DIV && expr2->cbcai && expr2->cbcaivalue == 0) // reference
    {
        fprintf(stderr, "类型检查错误：除数为0\n");
        exit(EXIT_FAILURE);
    }
    if (expr1->cbcai && expr2->cbcai)
    {
        cbcai = true;
        switch (op)
        {
        case ADD:
            cbcaivalue = expr1->cbcaivalue + expr2->cbcaivalue;
            break;
        case SUB:
            cbcaivalue = expr1->cbcaivalue - expr2->cbcaivalue;
            break;
        case MUL:
            cbcaivalue = expr1->cbcaivalue * expr2->cbcaivalue;
            break;
        case DIV:
            if (expr2->cbcaivalue == 0)
            {
                fprintf(stderr, "类型检查错误：除数为0\n");
                exit(EXIT_FAILURE);
            }
            cbcaivalue = expr1->cbcaivalue / expr2->cbcaivalue;
            break;
        case MOD:
            cbcaivalue = expr1->cbcaivalue % expr2->cbcaivalue;
            break;
        case AND:
            cbcaivalue = expr1->cbcaivalue && expr2->cbcaivalue;
            break;
        case OR:
            cbcaivalue = expr1->cbcaivalue || expr2->cbcaivalue;
            break;
        case LESS:
            cbcaivalue = expr1->cbcaivalue < expr2->cbcaivalue;
            break;
        case LESSEQUAL:
            cbcaivalue = expr1->cbcaivalue <= expr2->cbcaivalue;
            break;
        case GREATER:
            cbcaivalue = expr1->cbcaivalue > expr2->cbcaivalue;
            break;
        case GREATEREQUAL:
            cbcaivalue = expr1->cbcaivalue >= expr2->cbcaivalue;
            break;
        case EQUAL:
            cbcaivalue = expr1->cbcaivalue == expr2->cbcaivalue;
            break;
        case NOTEQUAL:
            cbcaivalue = expr1->cbcaivalue != expr2->cbcaivalue;
            break;
        }
    }
    if (expr1->getType()->isVoid())
    {
        // 如果 expr1 是 void 类型，报错或提示不能与 void 类型运算
        fprintf(stderr, "类型检查错误：不能与void类型进行运算\n");
        exit(EXIT_FAILURE);

        return; // 或者抛出异常
    }

    if (expr2->getType()->isVoid())
    {
        // 如果 expr2 是 void 类型，报错或提示不能与 void 类型运算
        fprintf(stderr, "类型检查错误：不能与void类型进行运算");
        exit(EXIT_FAILURE);

        return; // 或者抛出异常
    }
}
void UnaryExpr::typeCheck()
{
    // printf("UnaryExpr::typeCheck\n");
    expr->typeCheck();
    Type *type = expr->getSymbolEntry()->getType(); // 获取表达式类型

    // 如果表达式类型是void，报错
    if (type->isVoid())
    {
        fprintf(stderr, "类型检查错误：表达式类型为void\n");
        exit(EXIT_FAILURE);
    }
    else if (type->isFunc())
    {
        // 获取函数的返回值类型
        Type *returnType = dynamic_cast<FunctionType *>(type)->getRetType(); // 将type转换为其子类func类型，然后获取函数返回值的类型
        if (returnType->isVoid())
        {
            fprintf(stderr, "类型检查错误：函数返回值void参与单目运算\n");
            exit(EXIT_FAILURE);
        }
    }

    if (expr->cbcai)
    {
        cbcai = true;
        switch (op)
        {
        case POS:
            cbcaivalue = expr->cbcaivalue;
            break;
        case NEG:
            cbcaivalue = -expr->cbcaivalue;
            break;
        case NOT:
            cbcaivalue = !expr->cbcaivalue;
            break;
        }
    }
}
void Constant::typeCheck()
{
    // Todo
    if (symbolEntry->getType()->isInt())
    {
        this->cbcai = true;
        this->cbcaivalue = atoi(symbolEntry->toStr().c_str());
    }
    // printf("Constant::typeCheck\n");
}

void Id::typeCheck()
{
    // Todo
    // printf("Id::typeCheck\n");
    if (this->getSymbolEntry()->getType()->isConst())
    {
        this->cbcai = true;
        this->cbcaivalue = dynamic_cast<IdentifierSymbolEntry *>(this->getSymbolEntry())->ConstantValue;
    }
}

void IfStmt::typeCheck()
{
    // Todo
    cond->typeCheck();
    thenStmt->typeCheck();
    // cout << "IfStmt::typeCheck()" << endl;
    if (cond->getType()->isVoid())
    {
        // 如果条件不是布尔类型，则输出错误并抛出异常
        fprintf(stderr, "类型检查错误：if的cond为void类型\n");
        exit(EXIT_FAILURE);
    }
}

void IfElseStmt::typeCheck()
{
    // Todo
    cond->typeCheck();
    thenStmt->typeCheck();
    elseStmt->typeCheck();
    // cout << "IfElseStmt::typeCheck()" << endl;
}
void WhileStmt::typeCheck()
{
    // Todo
    cond->typeCheck();
    // isInwhile = true;
    stmt->typeCheck();
    // isInwhile = false;
    // cout << "WhileStmt::typeCheck()" << endl;
}
void BreakStmt::typeCheck()
{
    // if (!isInwhile)
    // {
    //     fprintf(stderr, "break should in whileStmt\n");
    //     exit(-1);
    // }
}

void ContinueStmt::typeCheck()
{
    // if (!isInwhile)
    // {
    //     fprintf(stderr, "continue should in whileStmt\n");
    //     exit(-1);
    // }
}

void CompoundStmt::typeCheck()
{
    // Todo
    // fprintf(stderr, "CompoundStmt::typeCheck()\n");
    stmt->typeCheck();
}

void SeqNode::typeCheck()
{
    // Todo
    // 检查 stmt1 的类型
    for (auto p : stmts)
    {
        p->typeCheck();
        // fprintf(stderr, "已检查\n");
    }
}

void DeclStmt::typeCheck()
{
    // 获取变量符号条目
    IdentifierSymbolEntry *se = dynamic_cast<IdentifierSymbolEntry *>(id->getSymPtr());

    // 如果存在后继节点，则继续检查
    if (next != nullptr)
    {
        next->typeCheck();
    }

    // 检查变量声明的类型
    if (se == nullptr)
    {
        throw std::runtime_error("SymbolEntry is not a valid IdentifierSymbolEntry");
    }

    // 如果表达式不为空，则检查表达式的类型
    if (expr != nullptr)
    {
        Type *exprType = expr->getType();

        // 如果表达式是一个函数调用，需要检查返回类型是否为 void
        if (exprType->isVoid())
        {
            // 输出错误信息
            fprintf(stderr, "类型检查错误：不能与void类型进行运算\n");
            exit(EXIT_FAILURE);
        }

        // 检查变量类型和表达式类型是否匹配
    }
}

void ReturnStmt::typeCheck()
{
    hasRet = true;

    if (returnType == nullptr)
    { // not in a fuction
        fprintf(stderr, "return statement outside functions\n");
        exit(EXIT_FAILURE);
    }
    else if (returnType->isVoid() && retValue != nullptr)
    { // returned a value in void()
        fprintf(stderr, "类型检查错误：返回类型错误\n");
        exit(EXIT_FAILURE);
    }
    else if (!returnType->isVoid() && retValue == nullptr)
    { // expected returned value, but returned nothing
        fprintf(stderr, "expected a %s type to return, but returned nothing\n", returnType->toStr().c_str());
        exit(EXIT_FAILURE);
    }
    if (!returnType->isVoid())
    {
        retValue->typeCheck();
    }
    if (retValue->getType()->isVoid() && !returnType->isVoid())
    {
        fprintf(stderr, "类型检查错误：返回类型错误\n");
        exit(EXIT_FAILURE);
    }
    if (retValue->getType()->isInt() && returnType->isVoid())
    {
        fprintf(stderr, "类型检查错误：返回类型错误\n");
        exit(EXIT_FAILURE);
    }
    this->retType = returnType;
    funcReturned = true;
}

void AssignStmt::typeCheck()
{
    if (expr->getType()->isFunc() && ((FunctionType *)(expr->getType()))->getRetType()->isVoid())
    { // 返回值为void的函数做运算数
        fprintf(stderr, "expected a return value, but functionType %s returns nothing\n", expr->getType()->toStr().c_str());
        exit(EXIT_FAILURE);
    }
    // Todo
    // cout << "AssignStmt::typeCheck" << endl;
    lval->typeCheck();
    expr->typeCheck();
}
void Node::setNext(Node *node)
{
    Node *temp = this;
    while (temp->getNext())
    {
        temp = temp->getNext();
    }
    temp->next = node;
}

Node *Node::getNext()
{
    return next;
}

void BinaryExpr::output(int level)
{
    std::string op_str;
    switch (op)
    {
    case ADD:
        op_str = "add";
        break;
    case SUB:
        op_str = "sub";
        break;
    case MUL:
        op_str = "mult";
        break;
    case DIV:
        op_str = "div";
        break;
    case MOD:
        op_str = "mod";
        break;
    case AND:
        op_str = "and";
        break;
    case OR:
        op_str = "or";
        break;
    case ASSIGN:
        op_str = "assign";
        break;
    case EQUAL:
        op_str = "equal";
        break;
    case NOTEQUAL:
        op_str = "notequal";
        break;
    case LESS:
        op_str = "less";
        break;
    case GREATER:
        op_str = "greater";
        break;
    case LESSEQUAL:
        op_str = "lessequal";
        break;
    case GREATEREQUAL:
        op_str = "greaterequal";
        break;
    }
    fprintf(yyout, "%*cBinaryExpr\top: %s\n", level, ' ', op_str.c_str());
    expr1->output(level + 4);
    expr2->output(level + 4);
}

void Ast::output()
{
    fprintf(yyout, "program\n");
    if (root != nullptr)
        root->output(4);
}

void Constant::output(int level)
{
    std::string type, value;
    type = symbolEntry->getType()->toStr();
    value = symbolEntry->toStr();
    fprintf(yyout, "%*cIntegerLiteral\tvalue: %s\ttype: %s\n", level, ' ',
            value.c_str(), type.c_str());
}

void ConstExpr::output(int level)
{
    std::string type, value;
    type = symbolEntry->getType()->toStr();
    value = symbolEntry->toStr(); // 这里可以获取常量的具体值
    fprintf(yyout, "%*cConstExpr\tvalue: %s\ttype: %s\n", level, ' ',
            value.c_str(), type.c_str());
}

void Float::output(int level)
{
    std::string type, value;
    type = symbolEntry->getType()->toStr();
    value = symbolEntry->toStr();
    fprintf(yyout, "%*cFloat\tvalue: %s\ttype: %s\n", level, ' ',
            value.c_str(), type.c_str());
}
void Float::typeCheck()
{
    // 对于 Float 类型的节点，可以暂时不做任何检查
}

void Float::genCode()
{
    // 目前不生成任何代码
}
void Bool::output(int level)
{
    std::string type, value;
    type = symbolEntry->getType()->toStr();
    value = symbolEntry->toStr();
    fprintf(yyout, "%*cBool\tvalue: %s\ttype: %s\n", level, ' ',
            value.c_str(), type.c_str());
}
void Bool::typeCheck()
{
    // 对于 Bool 类型的节点，可以暂时不做任何检查
}

void Bool::genCode()
{
    // 目前不生成任何代码
}
void Id::output(int level)
{
    std::string name, type;
    int scope;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    scope = dynamic_cast<IdentifierSymbolEntry *>(symbolEntry)->getScope();
    fprintf(yyout, "%*cId\tname: %s\tscope: %d\ttype: %s\n", level, ' ',
            name.c_str(), scope, type.c_str());
}

void CompoundStmt::output(int level)
{
    fprintf(yyout, "%*cCompoundStmt\n", level, ' ');
    stmt->output(level + 4);
}

void ExprStmt::output(int level)
{
    fprintf(yyout, "%*cExprStmt\n", level, ' ');
    expr->output(level + 4);
}

void EmptyStmt::output(int level)
{
    fprintf(yyout, "%*cEmptyStmt\n", level, ' ');
}

void SeqNode::output(int level)
{
    for (auto p : stmts)
    {
        p->output(level);
    }
}

void DeclStmt::output(int level)
{
    fprintf(yyout, "%*cDeclStmt\n", level, ' ');
    id->output(level + 4);
}

void IfStmt::output(int level)
{
    fprintf(yyout, "%*cIfStmt\n", level, ' ');
    cond->output(level + 4);
    thenStmt->output(level + 4);
}

void IfElseStmt::output(int level)
{
    fprintf(yyout, "%*cIfElseStmt\n", level, ' ');
    cond->output(level + 4);
    thenStmt->output(level + 4);
    elseStmt->output(level + 4);
}

void WhileStmt::output(int level)
{
    fprintf(yyout, "%*cWhileStmt\n", level, ' ');
    cond->output(level + 4);
    stmt->output(level + 4);
}

void ReturnStmt::output(int level)
{
    fprintf(yyout, "%*cReturnStmt\n", level, ' ');
    retValue->output(level + 4);
}
AssignStmt::AssignStmt(ExprNode *lval, ExprNode *expr) : lval(lval), expr(expr)
{
    Type *type = ((Id *)lval)->getType();
    Type *exprType = expr->getType();

    SymbolEntry *se = lval->getSymbolEntry();
    bool flag = true;
    if (((IntType *)type)->isConst())
    {
        fprintf(stderr, "cannot assign to variable \'%s\' with const-qualified type \'%s\'\n", ((IdentifierSymbolEntry *)se)->toStr().c_str(), type->toStr().c_str());
        flag = false;
    }
    else
    {
        ((IdentifierSymbolEntry *)se)->setValue(expr->getValue());
    }

    if (flag)
    {
        if (type != exprType)
        {
            fprintf(stderr, "cannot initialize a variable of type \'%s\' with an rvalue of type \'%s\'\n", type->toStr().c_str(), exprType->toStr().c_str());
        }
    }
}
void AssignStmt::output(int level)
{
    fprintf(yyout, "%*cAssignStmt\n", level, ' ');
    lval->output(level + 4);
    expr->output(level + 4);
}
void UnaryExpr::output(int level)
{
    std::string op_str;
    switch (op)
    {
    case POS:
        op_str = "pos";
        break;
    case NEG:
        op_str = "neg";
        break;
    case NOT:
        op_str = "not";
        break;
    }
    fprintf(yyout, "%*cUnaryExpr\top: %s\ttype: %s\n", level, ' ', op_str.c_str(), symbolEntry->getType()->toStr().c_str());
    expr->output(level + 4);
}
void FuncCallExp::output(int level)
{
    string name, type;
    int scope;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    scope = dynamic_cast<IdentifierSymbolEntry *>(symbolEntry)->getScope();
    fprintf(yyout, "%*cFuncCallExpr\tfunction name: %s\tscope: %d\ttype: %s\n",
            level, ' ', name.c_str(), scope, type.c_str());
    Node *temp = param;
    while (temp)
    {
        temp->output(level + 4);
        temp = temp->getNext();
    }
}

void FunctionDef::output(int level)
{
    std::string name, type;
    name = se->toStr();
    type = se->getType()->toStr();
    fprintf(yyout, "%*cFunctionDefine function name: %s, type: %s\n", level, ' ',
            name.c_str(), type.c_str());
    if (name == "main")
    {
        cout << "7897";
    }
    if (FuncDefParams)
    {
        FuncDefParams->output(level + 4);
    }
    stmt->output(level + 4);
}

void BreakStmt::output(int level)
{
    fprintf(yyout, "%*cBreakStmt\n", level, ' ');
}

void ContinueStmt::output(int level)
{
    fprintf(yyout, "%*cContinueStmt\n", level, ' ');
}
