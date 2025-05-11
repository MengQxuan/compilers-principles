#ifndef __UNIT_H__
#define __UNIT_H__

#include <vector>
#include "Function.h"
#include "AsmBuilder.h"

class Unit
{
    typedef std::vector<Function *>::iterator iterator;
    typedef std::vector<Function *>::reverse_iterator reverse_iterator;

private:
    std::vector<Function *> func_list;
    std::vector<GlobalVarDefInstruction *> global_var;

public:
    Unit() = default;
    ~Unit();
    void insertFunc(Function *);
    void removeFunc(Function *);
    void insertGlobalVar(GlobalVarDefInstruction *i) { global_var.push_back(i); }
    void removeGlobalVar(GlobalVarDefInstruction *i) { global_var.erase(std::find(global_var.begin(), global_var.end(), i)); }
    void output() const;
    void initLibraryFunctions();
    iterator begin() { return func_list.begin(); };
    iterator end() { return func_list.end(); };
    reverse_iterator rbegin() { return func_list.rbegin(); };
    reverse_iterator rend() { return func_list.rend(); };
    void removeUnusedAlloca();
    void samebboptimize();
    void genMachineCode(MachineUnit* munit);
};

#endif