#ifndef __OPERAND_H__
#define __OPERAND_H__

#include "SymbolTable.h"
#include <vector>

class Instruction;
class Function;

// class Operand - The operand of an instruction.
class Operand
{
    typedef std::vector<Instruction *>::iterator use_iterator;

private:
    Instruction *def;                // The instruction where this operand is defined.
    std::vector<Instruction *> uses; // Intructions that use this operand.
    SymbolEntry *se;                 // The symbol entry of this operand.
    std::vector<Instruction *> loaduses;
public:
    Operand(SymbolEntry *se) : se(se) { def = nullptr; };
    void setDef(Instruction *inst) { def = inst; };
    void addUse(Instruction *inst) { uses.push_back(inst); };
    void removeUse(Instruction *inst);
    int usersNum() const { return uses.size(); };
    void addloadUse(Instruction *inst) { loaduses.push_back(inst); };
    int loadusersNum() const { return loaduses.size(); };
    Instruction *getDef() { return def; };
    std::vector<Instruction *> &getUse() { return uses; };
    SymbolEntry* getSymPtr(){return se;}

    use_iterator use_begin() { return uses.begin(); };
    use_iterator use_end() { return uses.end(); };
    Type *getType() { return se->getType(); };
    std::string toStr() const;
    SymbolEntry * getEntry() { return se; };
    bool isConst()
    {
        if (!se->isVariable())
            return false;
        else
            return ((IdentifierSymbolEntry *)se)->isConstant();
    }
};

#endif