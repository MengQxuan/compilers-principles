#include "Type.h"
#include <sstream>

// 基本类型的初始化
IntType TypeSystem::commonInt = IntType(32);
IntType TypeSystem::commonBool = IntType(1);
VoidType TypeSystem::commonVoid = VoidType();
DecimalType TypeSystem::commonDecimal(4);

FloatType TypeSystem::commonFloat(4);

ArrayType TypeSystem::commonArray(10);

ConstType TypeSystem::commonConst(&commonInt); // commonConst 必须在 commonInt 之后定义

// 指针类型的初始化
Type *TypeSystem::intType = &commonInt;
Type *TypeSystem::voidType = &commonVoid;
Type *TypeSystem::decimalType = &commonDecimal;

Type *TypeSystem::floatType = &commonFloat;

Type *TypeSystem::arrayType = &commonArray;
Type *TypeSystem::constType = &commonConst; // 这里只定义一次
Type *TypeSystem::boolType = &commonBool;

// `ConstType` 的获取方法
Type *TypeSystem::getConstType(Type *baseType)
{
    return new ConstType(baseType);
}
bool Type::isValid(Type *t1, Type *t2)
{
    if (t1->isFloat() || t2->isFloat() || t1->isInt() || t2->isInt() || t1 == t2)
        return true;
    return false;
}
// 各种类型的 `toStr()` 方法实现
std::string IntType::toStr()
{
    std::ostringstream buffer;
    buffer << "i" << size;
    return buffer.str();
}

std::string VoidType::toStr()
{
    return "void";
}

std::string BoolType::toStr()
{
    return "bool";
}
std::string FunctionType::toStr()
{
    ostringstream buffer;
    buffer << returnType->toStr() << "(";
    for (auto it = paramsType.begin(); it != paramsType.end(); it++)
    {
        buffer << (*it)->toStr();
        if (it + 1 != paramsType.end())
        {
            buffer << ", ";
        }
    }
    buffer << ')';
    return buffer.str();
}
std::string DecimalType::toStr()
{
    return "decimal";
}



std::string FloatType::toStr()
{
    return "float";
}



std::string ConstType::toStr()
{
    return "const" + baseType->toStr();
}

std::string PointerType::toStr()
{
    std::ostringstream buffer;
    buffer << valueType->toStr() << "*";
    return buffer.str();
}
std::string ArrayType::toStr()
{
    return "array";
}

bool TypeSystem::needCast(Type *src, Type *target)
{
    if (src->isInt() && target->isInt())
    {
        return false;
    }
    if (src->isFloat() && target->isFloat())
    {
        return false;
    }
    if (src->isBool() && target->isBool())
    {
        return false;
    }
    return true;
}