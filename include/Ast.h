#ifndef __AST_H__
#define __AST_H__

#include <fstream>
#include "Operand.h"

class SymbolEntry;
class Unit;
class Function;
class BasicBlock;
class Instruction;
class IRBuilder;
class Node
{
private:
    static int counter;
    int seq;

protected:
    std::vector<Instruction *> true_list;
    std::vector<Instruction *> false_list;
    static IRBuilder *builder;
    void backPatch(std::vector<Instruction *> &list, BasicBlock *bb);
    std::vector<Instruction *> merge(std::vector<Instruction *> &list1, std::vector<Instruction *> &list2);

public:
    static void setIRBuilder(IRBuilder *ib) { builder = ib; };

    Node *next;
    Node();
    int getSeq() const { return seq; };
    virtual void output(int level) = 0;
    void setNext(Node *node);
    Node *getNext();
    virtual void typeCheck() = 0;
    virtual void genCode() = 0;
    std::vector<Instruction *> &trueList() { return true_list; }
    std::vector<Instruction *> &falseList() { return false_list; }
};

class ExprNode : public Node
{
protected:
    SymbolEntry *symbolEntry;

public:
    Operand *dst; // The result of the subtree is stored into dst.
    ExprNode(SymbolEntry *symbolEntry) : symbolEntry(symbolEntry) {};
    SymbolEntry *getSymbolEntry() const
    {
        return symbolEntry;
    }
    Operand *getOperand() { return dst; };
    Type *getType()
    {
        return symbolEntry->getType();
    }
    SymbolEntry *getSymPtr() { return symbolEntry; };
    void typeCheck();
    void genCode();
    // reference
    bool cbcai = false;
    int cbcaivalue;
    void toBool(Function *func);
    virtual int getValue() { return -1; };

    std::string getName() const { return symbolEntry->getName(); }
};

class BinaryExpr : public ExprNode
{
private:
    int op;
    ExprNode *expr1, *expr2;

public:
    enum binaryOp
    {
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        AND,
        OR,
        ASSIGN,
        LESS,
        LESSEQUAL,
        GREATER,
        GREATEREQUAL,
        EQUAL,
        NOTEQUAL
    };
    BinaryExpr(SymbolEntry *se, int op, ExprNode *expr1, ExprNode *expr2) : ExprNode(se), op(op), expr1(expr1), expr2(expr2) { dst = new Operand(se); };
    void output(int level);
    void typeCheck();
    void genCode();
    int getValue();
};
class UnaryExpr : public ExprNode
{
private:
    int op;
    ExprNode *expr;

public:
    enum
    {
        NOT, // 逻辑非操作
        NEG, // 取负操作
        POS, // 正号操作
    };

    UnaryExpr(SymbolEntry *se, int op, ExprNode *expr)
        : ExprNode(se), op(op), expr(expr)
    {
        // if (expr->getOperand()->getType() == TypeSystem::boolType && op == UnaryExpr::NEG)
        // {
        //     return;
        // }

        dst = new Operand(se); // 创建目标操作数
    }

    void output(int level);
    void typeCheck();
    void genCode();
    int getValue();
};
class Constant : public ExprNode
{
public:
    Constant(SymbolEntry *se) : ExprNode(se) { dst = new Operand(se); };
    void output(int level);
    void typeCheck();
    void genCode();
    int getValue();
};

class ConstExpr : public ExprNode
{
public:
    ConstExpr(SymbolEntry *se) : ExprNode(se) {}
    void output(int level) override;
    int getValue();
};

class Float : public ExprNode
{
public:
    Float(SymbolEntry *se) : ExprNode(se) {};
    void output(int level);
    void typeCheck();
    void genCode();
    int getValue(){return 0;};
};
class Bool : public ExprNode
{
public:
    Bool(SymbolEntry *se) : ExprNode(se) {};
    void output(int level);
    void typeCheck();
    void genCode();
    int getValue(){return 0;};
};
class Id : public ExprNode
{
public:
    Id(SymbolEntry *se) : ExprNode(se)
    {
        SymbolEntry *temp = new TemporarySymbolEntry(se->getType(), SymbolTable::getLabel());
        dst = new Operand(temp);
    };
    void output(int level);
    void typeCheck();
    void genCode();
    int getValue();
};

class StmtNode : public Node
{
};

class CompoundStmt : public StmtNode
{
private:
    StmtNode *stmt;

public:
    CompoundStmt(StmtNode *stmt) : stmt(stmt) {};
    void output(int level);
    void typeCheck();
    void genCode();
};
class ExprStmt : public StmtNode
{
private:
    ExprNode *expr;

public:
    ExprStmt(ExprNode *expr) : expr(expr) {};
    void output(int level);
    void typeCheck();
    void genCode();
};
class EmptyStmt : public StmtNode
{
public:
    EmptyStmt() {};
    void output(int level);
    void typeCheck();
    void genCode();
    
};

class SeqNode : public StmtNode
{
private:
    std::vector<StmtNode *> stmts;

public:
    SeqNode() {}
    void addStmtBack(StmtNode *stmt) { stmts.push_back(stmt); }
    void addStmtFront(StmtNode *stmt) { stmts.insert(stmts.begin(), stmt); }
    void output(int level);
    void typeCheck();
    void genCode();
};

class DeclStmt : public StmtNode
{
private:
    Id *id;
    ExprNode *expr;
    bool isParam;

public:
    DeclStmt(Id *id, ExprNode *expr = nullptr) : id(id), expr(expr) {};
    void output(int level);
    Id *getId() { return id; }
    void typeCheck();
    void genCode();
    void setIsParam() { isParam = true; }
};

class IfStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *thenStmt;

public:
    IfStmt(ExprNode *cond, StmtNode *thenStmt) : cond(cond), thenStmt(thenStmt) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class IfElseStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *thenStmt;
    StmtNode *elseStmt;

public:
    IfElseStmt(ExprNode *cond, StmtNode *thenStmt, StmtNode *elseStmt) : cond(cond), thenStmt(thenStmt), elseStmt(elseStmt) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class WhileStmt : public StmtNode
{
public:
    WhileStmt(ExprNode *cond, StmtNode *stmt) : cond(cond), stmt(stmt) {}
    void output(int level) override;

private:
    ExprNode *cond;
    StmtNode *stmt;
    void typeCheck();
    void genCode();
};

class ReturnStmt : public StmtNode
{
private:
    ExprNode *retValue;
    Type *retType;

public:
    ReturnStmt(ExprNode *retValue) : retValue(retValue) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class AssignStmt : public StmtNode
{
private:
    ExprNode *lval;
    ExprNode *expr;

public:
    AssignStmt(ExprNode *lval, ExprNode *expr);
    void output(int level);
    void typeCheck();
    void genCode();
};

class FuncCallParamsNode;
class FuncCallExp : public ExprNode
{
private:
    SymbolEntry *funcSym;
    ExprNode *param;

public:
    FuncCallExp(SymbolEntry *se, SymbolEntry *res, ExprNode *param = nullptr)
        : ExprNode(res), funcSym(se), param(param) { dst = new Operand(res); };
    void output(int level);
    void typeCheck();
    void genCode();
};

class FunctionDef : public StmtNode
{
private:
    SymbolEntry *se;
    StmtNode *stmt;
    DeclStmt *FuncDefParams;
    StmtNode *voidAddRet = nullptr;

public:
    FunctionDef(SymbolEntry *se, StmtNode *stmt, DeclStmt *FuncDefParams = nullptr) : se(se), stmt(stmt), FuncDefParams(FuncDefParams) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class BreakStmt : public StmtNode
{
public:
    BreakStmt() {};
    void output(int level) override;
    void typeCheck();
    void genCode();
};

class ContinueStmt : public StmtNode
{
public:
    ContinueStmt() {};
    void output(int level) override;
    void typeCheck();
    void genCode();
};

class Ast
{
private:
    Node *root;

public:
    Ast() { root = nullptr; }
    void setRoot(Node *n) { root = n; }
    void output();
    void typeCheck();
    void genCode(Unit *unit);
};

#endif
