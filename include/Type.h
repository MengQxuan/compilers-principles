#ifndef __TYPE_H__
#define __TYPE_H__

#include <vector>
#include <string>
using namespace std;

// 基类 Type
class Type
{
private:
    int kind;

protected:
    enum
    {
        INT,
        VOID,
        FUNC,
        BOOL,
        PTR,
        DECIMAL,
        OCTAL,
        HEXADECIMAL,
        FLOAT,
        OCTAL_FLOAT,
        HEX_FLOAT,
        CONST,
        ARRAY
    }; // 添加新的类型

public:
    int size;
    Type(int kind,int size) : kind(kind),size(size) {};
    virtual ~Type() {};
    virtual std::string toStr() = 0;

    bool isInt() const { return kind == INT; };
    bool isVoid() const { return kind == VOID; };
    bool isFunc() const { return kind == FUNC; };
    // bool isDecimal() const { return kind == DECIMAL; }
    // bool isOctal() const { return kind == OCTAL; }
    // bool isHexadecimal() const { return kind == HEXADECIMAL; }
    bool isFloat() const { return kind == FLOAT; }
    // bool isOctalFloat() const { return kind == OCTAL_FLOAT; }
    // bool isHexFloat() const { return kind == HEX_FLOAT; }
    bool isConst() const { return kind == CONST; }
    bool isBool() const { return kind == BOOL; }
    bool calculatable() const { return isInt() || isFloat() || isBool(); } // 不是void其实就行
    // bool isValid(Type *t1, Type *t2);
    static bool isValid(Type *t1, Type *t2);
    int get(){return kind;}
    int getSize() const { return size; };
    bool isPtr() const { return kind == PTR; };
};

// 各种类型的定义
class IntType : public Type
{
private:
    // int size;

public:
    IntType(int size) : Type(Type::INT,size) {};
    std::string toStr();
};
class BoolType : public Type
{
private:
    // int size;

public:
    BoolType(int size, bool is_const = false) : Type(Type::BOOL,size) {};
    std::string toStr() override;
};
class VoidType : public Type
{
public:
    VoidType() : Type(Type::VOID,0) {};
    std::string toStr();
};

class FunctionType : public Type
{
private:
    Type *returnType;          // 返回值类型
    vector<Type *> paramsType; // 参数类型
public:
    FunctionType(Type *returnType, vector<Type *> paramsType) : Type(Type::FUNC,0), returnType(returnType), paramsType(paramsType) {};
    void setParamsType(vector<Type *> p) { paramsType = p; }
    Type *getParamsType(unsigned int index)
    {
        if (index < paramsType.size())
            return paramsType[index];
        else
            return nullptr;
    }
    Type *getRetType() { return returnType; };
    std::string toStr();
};

class DecimalType : public Type
{
private:
    // int size;

public:
    DecimalType(int size) : Type(Type::DECIMAL,size) {}
    std::string toStr() override;
};





class FloatType : public Type
{
private:
    // int size;

public:
    FloatType(int size) : Type(Type::FLOAT,size) {}
    std::string toStr() override;
};





class ConstType : public Type
{
private:
    Type *baseType; // const 类型的基础类型

public:
    ConstType(Type *baseType) : Type(Type::CONST,32), baseType(baseType) {} // 设置为 CONST 类型，并记录基础类型
    std::string toStr() override;
    Type *getBaseType() const { return baseType; } // 获取基础类型
};

class ArrayType : public Type
{
private:
    // int size;

public:
    ArrayType(int size) : Type(Type::ARRAY,0) {} // 将类型设置为 ARRAY
    std::string toStr() override;
};
class PointerType : public Type
{
private:
    Type *valueType;

public:
    PointerType(Type *valueType) : Type(Type::PTR,0) { this->valueType = valueType; };
    std::string toStr();

    Type *getValueType() { return valueType; }
};
class TypeSystem
{
private:
    // 常用类型的静态实例
    static IntType commonInt;
    static IntType commonBool;
    static BoolType commonConstBool;

    static VoidType commonVoid;
    static DecimalType commonDecimal;

    static FloatType commonFloat;

    static ArrayType commonArray;
    static ConstType commonConst;

public:
    static Type *intType;
    static Type *voidType;
    static Type *boolType;

    static Type *decimalType;
    static Type *floatType;
    static Type *octalFloatType;
    static Type *hexFloatType;
    static Type *constType;
    static Type *arrayType;

    static Type *getConstType(Type *baseType); // 获取常量类型
    static bool needCast(Type *type1, Type *type2);
};

#endif // __TYPE_H__
