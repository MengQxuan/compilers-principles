#ifndef __SYMBOLTABLE_H__
#define __SYMBOLTABLE_H__

#include <string>
#include <map>
#include <vector>
#include "Type.h"
class Type;
class Operand;

class SymbolEntry
{
private:
    int kind;

protected:
    enum
    {
        CONSTANT,
        VARIABLE,
        TEMPORARY,
    };
    Type *type;
    std::string name;

    union
    {
        int intValue;
        float floatValue;
        // bool boolValue;
    } value;

public:
    SymbolEntry(Type *type, int kind);
    virtual ~SymbolEntry() {};
    bool isConstant() const { return kind == CONSTANT; };
    bool isId() const { return kind == VARIABLE; };
    bool isBool() const { return type->isBool(); };
    bool isTemporary() const { return kind == TEMPORARY; };
    bool isVariable() const { return kind == VARIABLE; };
    Type *getType() { return type; };
    void setType(Type *type) { this->type = type; };
    virtual std::string toStr() = 0;
    // You can add any function you need here.
    SymbolEntry(std::string name) : name(name) {}
    std::string getName() const { return name; }
    // int getParamNo(){return paramNo;};
    float getFloat() { return value.floatValue; }
    int getInt() { return value.intValue; }
    void setValue(float f)
    {
        value.floatValue = f;
    }
    int getValue() { return value.intValue; }
    // void setboolValue(bool a) { value.boolValue = a; }
};

/*
    Symbol entry for literal constant. Example:

    int a = 1;

    Compiler should create constant symbol entry for literal constant '1'.
*/
// 用于库函数的 SymbolEntry 子类
// class FunctionSymbolEntry : public SymbolEntry
// {
// private:
//     std::string name;
//     std::vector<Type *> paramTypes;
//     Type *returnType;

// public:
//     FunctionSymbolEntry(Type *returnType, const std::vector<Type *> &paramTypes, const std::string &name)
//         : SymbolEntry(returnType, VARIABLE), name(name), paramTypes(paramTypes), returnType(returnType) {}

//     std::string getName() const { return name; }
//     Type *getReturnType() const { return returnType; }
//     const std::vector<Type *> &getParamTypes() const { return paramTypes; }
//     std::string toStr() override;
// };

class ConstantSymbolEntry : public SymbolEntry
{
private:
    int value;

public:
    ConstantSymbolEntry(Type *type, int value);
    virtual ~ConstantSymbolEntry() {};
    int getValue() const { return value; };
    std::string toStr();
    void setValue(int a) { value = a; }
    // You can add any function you need here.
};

class FloatSymbolEntry : public SymbolEntry
{
private:
    double value;

public:
    FloatSymbolEntry(Type *type, double value);
    virtual ~FloatSymbolEntry() {};
    double getValue() const { return value; };
    std::string toStr();
    // You can add any function you need here.
};
class BoolSymbolEntry : public SymbolEntry
{
private:
    bool value;

public:
    BoolSymbolEntry(Type *type, bool value);
    virtual ~BoolSymbolEntry() {};
    double getValue() const { return value; };
    std::string toStr();
    // You can add any function you need here.
};
/*
    Symbol entry for identifier. Example:

    int a;
    int b;
    void f(int c)
    {
        int d;
        {
            int e;
        }
    }

    Compiler should create identifier symbol entries for variables a, b, c, d and e:

    | variable | scope    |
    | a        | GLOBAL   |
    | b        | GLOBAL   |
    | c        | PARAM    |
    | d        | LOCAL    |
    | e        | LOCAL +1 |
*/
class IdentifierSymbolEntry : public SymbolEntry
{
private:
    enum
    {
        GLOBAL,
        PARAM,
        LOCAL
    };
    std::string name;
    int scope;
    Operand *addr; // The address of the identifier.
    // You can add any field you need here.
    bool isConst; // 新增成员变量，用于标识是否为const
    bool isid;

public:
    int ConstantValue;
    IdentifierSymbolEntry(Type *type, std::string name, int scope, bool isConst = false);
    virtual ~IdentifierSymbolEntry() {};
    std::string toStr();
    bool isGlobal() const { return scope == GLOBAL; };
    bool isParam() const { return scope == PARAM; };
    bool isLocal() const { return scope >= LOCAL; };
    void setValue(int value){this->ConstantValue = value;};
    int getScope() const { return scope; };
    void setAddr(Operand *addr) { this->addr = addr; };
    Operand *getAddr() { return addr; };         // You can add any function you need here.
    bool isConstant() const { return isConst; }; // 新增方法用于判断是否为const
    bool isId() const { return isid; };          // 新增方法用于判断是否为id
    int getValue() { return ConstantValue; }
    
    // void outputFuncDecl();                       // 输出
    void setId()
    {
        isid = true;
    }
    void setConst()
    {
        isConst = true;
        // hasValue = true;
    }
};

/*
    Symbol entry for temporary variable created by compiler. Example:

    int a;
    a = 1 + 2 + 3;

    The compiler would generate intermediate code like:

    t1 = 1 + 2
    t2 = t1 + 3
    a = t2

    So compiler should create temporary symbol entries for t1 and t2:

    | temporary variable | label |
    | t1                 | 1     |
    | t2                 | 2     |
*/
class TemporarySymbolEntry : public SymbolEntry
{
private:
    int stack_offset;
    int label;

public:
    TemporarySymbolEntry(Type *type, int label);
    virtual ~TemporarySymbolEntry() {};
    std::string toStr();
    int getLabel() const { return label; };
    void setOffset(int offset) { this->stack_offset = offset; };
    int getOffset() { return this->stack_offset; };
    // You can add any function you need here.
};

// symbol table managing identifier symbol entries
class SymbolTable
{
private:
    std::map<std::string, SymbolEntry *> symbolTable;
    SymbolTable *prev;
    int level;
    static int counter;

public:
    SymbolTable();
    SymbolTable(SymbolTable *prev);
    void install(std::string name, SymbolEntry *entry);
    SymbolEntry *lookup(std::string name);
    SymbolEntry *lookupCurrentLevel(std::string name);
    SymbolTable *getPrev() { return prev; };
    int getLevel() { return level; };
    static int getLabel() { return counter++; };
    bool checkExist(std::string name);
};

extern SymbolTable *identifiers;
extern SymbolTable *globals;

#endif
