#include "SymbolTable.h"
#include "Type.h"
#include <iostream>
#include <vector>
#include <sstream>

SymbolEntry::SymbolEntry(Type *type, int kind)
{
    this->type = type;
    this->kind = kind;
}

ConstantSymbolEntry::ConstantSymbolEntry(Type *type, int value) : SymbolEntry(type, SymbolEntry::CONSTANT)

{
    this->value = value;
}

std::string ConstantSymbolEntry::toStr()
{
    std::ostringstream buffer;
    buffer << value;
    return buffer.str();
}
FloatSymbolEntry::FloatSymbolEntry(Type *type, double value)
    : SymbolEntry(type, SymbolEntry::CONSTANT)
{
    this->value = value;
}
std::string FloatSymbolEntry::toStr()
{
    std::ostringstream buffer;
    buffer << value;
    return buffer.str();
}
BoolSymbolEntry::BoolSymbolEntry(Type *type, bool value)
    : SymbolEntry(type, SymbolEntry::CONSTANT)
{
    this->value = value;
}
std::string BoolSymbolEntry::toStr()
{
    std::ostringstream buffer;
    buffer << value;
    return buffer.str();
}

IdentifierSymbolEntry::IdentifierSymbolEntry(Type *type, std::string name, int scope, bool isConst)
    : SymbolEntry(type, SymbolEntry::VARIABLE), name(name), scope(scope), isConst(isConst)
{
    this->scope = scope;
    addr = nullptr;
}

std::string IdentifierSymbolEntry::toStr()
{
    return "@" + name;
}

TemporarySymbolEntry::TemporarySymbolEntry(Type *type, int label) : SymbolEntry(type, SymbolEntry::TEMPORARY)
{
    this->label = label;
}

std::string TemporarySymbolEntry::toStr()
{
    std::ostringstream buffer;
    buffer << "%t" << label;
    return buffer.str();
}

SymbolTable::SymbolTable(SymbolTable *prev)
{
    this->prev = prev;
    this->level = prev->level + 1;
}

SymbolEntry *SymbolTable::lookup(std::string name)
{
    if (symbolTable.find(name) != symbolTable.end())
    {
        return symbolTable[name];
    }
    else
    {
        if (prev != nullptr)
        {
            return prev->lookup(name);
        }
        else
        {
            return nullptr;
        }
    }
}
SymbolEntry *SymbolTable::lookupCurrentLevel(std::string name)
{
    // 只查找当前作用域的 symbolTable，而不递归到上级作用域
    if (symbolTable.find(name) != symbolTable.end())
    {
        return symbolTable[name]; // 找到返回 SymbolEntry
    }
    else
    {
        return nullptr; // 没找到返回 nullptr
    }
}

int SymbolTable::counter = 0;
static SymbolTable t;
SymbolTable *identifiers = &t;
SymbolTable *globals = &t;

// std::string FunctionSymbolEntry::toStr()
// {
//     std::string str = name + "(";
//     for (size_t i = 0; i < paramTypes.size(); ++i)
//     {
//         str += paramTypes[i]->toStr();
//         if (i < paramTypes.size() - 1)
//             str += ", ";
//     } // 输出所有参数类型
//     str += ") -> " + returnType->toStr(); // 返回类型
//     return str;
// }

// install the entry into current symbol table.
void SymbolTable::install(std::string name, SymbolEntry *entry)
{
    symbolTable[name] = entry;
}

SymbolTable::SymbolTable()
{
    // printf("putint已完成");
    prev = nullptr;
    level = 0;

    // // 创建标准库函数 putint(int) 并添加到符号表
    // Type *intType = TypeSystem::intType; // commonInt是一个静态变量，不应该被修改
    // // 此处应该新建一个intType，而TypeSystem::intType = &commonInt，IntType TypeSystem::commonInt = IntType(4);  则TypeSystem::intType就是一个IntType(4)
    // Type *voidType = TypeSystem::voidType;
    // std::vector<Type *> paramTypes = {intType};

    // FunctionSymbolEntry *putintEntry = new FunctionSymbolEntry(voidType, paramTypes, "putint");
    // install("putint", putintEntry);

    // // 创建标准库函数 getint() 并添加到符号表
    // FunctionSymbolEntry *getintEntry = new FunctionSymbolEntry(intType, {}, "getint");
    // install("getint", getintEntry);
}

bool SymbolTable::checkExist(std::string name)
{
    if (symbolTable.find(name) != symbolTable.end())
        return true;
    else
        return false;
}