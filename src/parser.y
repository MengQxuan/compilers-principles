%code top{
    #include <iostream>
    #include <assert.h>
    #include "parser.h"
    extern Ast ast;
    int yylex();
    int yyerror( char const * );
    
}


%code requires {
    #include "Ast.h"
    #include "SymbolTable.h"
    #include "Type.h"
}

%union {
    int itype;
    char* strtype;
    StmtNode* stmttype;
    ExprNode* exprtype;
    Type* type;
    float ftype;
    bool btype;
}

%start Program
%token <strtype> ID 
%token <itype> INTEGER
%token <ftype> FLOAT_LITERAL
%token <itype> HEX_LITERAL
%token <itype> OCTAL_LITERAL
%token <btype> BOOL_LITERAL
%token IF ELSE WHILE
%token INT VOID DECIMAL OCTAL HEXADECIMAL FLOAT OCTAL_FLOAT HEX_FLOAT  BOOL
%token LPAREN RPAREN LBRACE RBRACE SEMICOLON COMMA
%token ADD SUB MUL DIV MOD NOT OR AND LESS GREATER ASSIGN EQUAL NOTEQUAL LESSEQUAL GREATEREQUAL  
%token RETURN BREAK CONTINUE
%token CONST

%nterm <stmttype> Stmts Stmt AssignStmt BlockStmt IfStmt WhileStmt ReturnStmt BreakStmt ContinueStmt EmptyStmt DeclStmt ConstDeclStmt FuncDef FuncDefParams FuncDefParam funcBlock ExprStmt  DeclStmtNode DeclStmtNodes
%nterm <exprtype> Exp AddExp Cond LOrExp PrimaryExp LVal RelExp LAndExp ExpList FuncCallExp UnaryExp MulExp
%nterm <type> Type

%precedence THEN
%precedence ELSE
%%
Program
    : Stmts {
        ast.setRoot($1);
    }
    ;
Stmts
    : Stmt {
        $$=new SeqNode();
        ((SeqNode*)$$)->addStmtBack($1);
    }
    | Stmts Stmt {
        $$ = $1;
        ((SeqNode*)$$)->addStmtBack($2);
    }
    ;
Stmt
    : AssignStmt { $$ = $1; }
    | BlockStmt { $$ = $1; }
    | IfStmt { $$ = $1; }
    | WhileStmt {$$=$1;}
    | ReturnStmt { $$ = $1; }
    | DeclStmt { $$ = $1; }
    | FuncDef { $$ = $1; }
    | EmptyStmt {$$=$1;}
    | ExprStmt {$$=$1;}
    | BreakStmt {$$=$1;}
    | ContinueStmt {$$=$1;}
    | ConstDeclStmt {$$=$1;}
    ;
LVal
    : ID {
        SymbolEntry *se;
        se = identifiers->lookup($1);
        //fprintf(stderr, "identifier \"%s\" is defined\n", (char*)$1);
        if (se == nullptr) {
            fprintf(stderr, "类型检查错误：变量 \"%s\" 未定义\n", (char*)$1);
            exit(EXIT_FAILURE);
            delete [](char*)$1;
            assert(se != nullptr);
        }
        if (se->isConstant()) {
            fprintf(stderr, "类型检查错误： 不能给给常量\"%s\" 赋值\n", (char*)$1);
            exit(EXIT_FAILURE);
        }
        $$ = new Id(se);
        delete []$1;
    }
    ;
AssignStmt
    :
    LVal ASSIGN Exp SEMICOLON {
        if ($1->getSymbolEntry()->isConstant()) {
            fprintf(stderr, "类型检查错误： 不能给给常量\"%s\" 赋值\n", $1->getSymbolEntry()->getName().c_str());
            $$ = nullptr;
        } else {
            $$ = new AssignStmt($1, $3);
        }
    }
    ;
BlockStmt
    :   LBRACE 
        {identifiers = new SymbolTable(identifiers);} 
        Stmts RBRACE 
        {
            $$ = new CompoundStmt($3);
            SymbolTable *top = identifiers;
            identifiers = identifiers->getPrev();
            delete top;
        }
    ;
IfStmt
    : IF LPAREN Cond RPAREN Stmt %prec THEN {
        $$ = new IfStmt($3, $5);
    }
    | IF LPAREN Cond RPAREN Stmt ELSE Stmt {
        $$ = new IfElseStmt($3, $5, $7);
    }
    ;
WhileStmt
    : WHILE LPAREN Cond RPAREN Stmt {
        $$ = new WhileStmt($3, $5);
    }
    ;
ReturnStmt
    :
    RETURN Exp SEMICOLON{
        $$ = new ReturnStmt($2);
    }
    ;
BreakStmt
    : BREAK SEMICOLON {$$ = new BreakStmt();}
    ;
ContinueStmt
    : CONTINUE SEMICOLON {$$ = new ContinueStmt();}
    ;
Exp
    :
    AddExp {$$ = $1;}
    ;
ExpList
    : %empty { $$ = nullptr; }
    | AddExp {
        $$ = $1;
    }
    | ExpList COMMA AddExp {
        $$ = $1;
        $$->setNext($3);
    }
    ;
ExprStmt
    : Exp SEMICOLON {$$ = new ExprStmt($1);}
    ;
EmptyStmt
    : SEMICOLON {$$ = new EmptyStmt();}
    | LBRACE RBRACE {$$ = new EmptyStmt();}
    ;
Cond
    :
    LOrExp {$$ = $1;}
    ;
PrimaryExp
    :
    LVal {
        $$ = $1;
    }
    |  LPAREN Exp RPAREN {
            $$ = $2;
        }
    | INTEGER {
        SymbolEntry *se = new ConstantSymbolEntry(TypeSystem::intType, $1);
        $$ = new Constant(se);
    }
    |  FLOAT_LITERAL {
        SymbolEntry *se = new FloatSymbolEntry(TypeSystem::floatType, $1);
        $$ = new Float(se);
    }
    | HEX_LITERAL {
        SymbolEntry *se = new ConstantSymbolEntry(TypeSystem::intType, $1);
        $$ = new Constant(se);
    }
    | OCTAL_LITERAL {
        SymbolEntry *se = new ConstantSymbolEntry(TypeSystem::intType, $1);
        $$ = new Constant(se);
    }
    | BOOL_LITERAL{
        SymbolEntry *se = new BoolSymbolEntry(TypeSystem::boolType, $1);
        $$ = new Bool(se);
    }
    | FuncCallExp { $$ = $1; }
    
    ;
AddExp
    :
    MulExp {$$ = $1;}
    |
    AddExp ADD MulExp
    {
        Type* leftType = $1->getType();
        Type* rightType = $3->getType();
        // 输出类型信息
        if (leftType && rightType) {
            //fprintf(stderr, "左操作数类型: %s\n", leftType->toStr().c_str());
            //fprintf(stderr, "右操作数类型: %s\n", rightType->toStr().c_str());
                      if (rightType->isVoid() || leftType->isVoid()) {
            fprintf(stderr, "类型检查错误：赋值为void\n");
            exit(EXIT_FAILURE);
            }
        }
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::ADD, $1, $3);
    }
    |
    AddExp SUB MulExp
    {
        
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::SUB, $1, $3);
    }
    ;
/* UnaryExp
    :PrimaryExp {$$ = $1;}
    | ADD UnaryExp {
    SymbolEntry *se = $2->getSymbolEntry();
    $$ = new UnaryExpr(se, UnaryExpr::POS, $2);//需要新建一个一元表达式对象，一元表达式需要一个单独的类，因为和二元表达式的符号含义不同
    }
    | SUB UnaryExp {
    SymbolEntry *se = $2->getSymbolEntry();
    $$ = new UnaryExpr(se, UnaryExpr::NEG, $2);
    }
    | NOT UnaryExp {
    SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
    $$ = new UnaryExpr(se, UnaryExpr::NOT, $2);
    }
; */
UnaryExp
    :PrimaryExp {$$ = $1;}
    | ADD UnaryExp {
    SymbolEntry *se;

    // 检查当前表达式是否为常量
    if ($2->getSymbolEntry()->isConstant()) {
        // 如果是常量，进行常量折叠
        ConstantSymbolEntry* constEntry = dynamic_cast<ConstantSymbolEntry*>($2->getSymbolEntry());
        if (constEntry) {
            // 直接使用常量的值作为结果
            if (constEntry->getType()->isInt()) {
                int a = constEntry->getValue();
                constEntry->setValue(a); // 加法操作在这里没有实际改变值
            }
            $$ = $2; // 使用更新后的常量表达式作为结果
        } else {
            // 常量转换失败，抛出错误或进行其他处理
            $$ = nullptr; // 错误处理，返回空指针
        }
    } else if ($2->getSymbolEntry()->isId()) {
        // 如果是标识符，获取其值并进行处理
        IdentifierSymbolEntry* idEntry = dynamic_cast<IdentifierSymbolEntry*>($2->getSymbolEntry());
        if (idEntry) {
            if (idEntry->getType()->isInt()) {
                int a = idEntry->getValue();
                //std::cout << "Current value of identifier: " << a << std::endl;
                idEntry->setValue(a); // 加法操作没有改变值
            }
            $$ = $2; // 使用更新后的标识符作为结果
        } else {
            // 标识符转换失败，抛出错误或进行其他处理
            $$ = nullptr; // 错误处理，返回空指针
        }
    } else {
        // 如果不是常量也不是标识符，则需要创建一个新的临时符号项
        se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new UnaryExpr(se, UnaryExpr::POS, $2); // 创建一个新的一元表达式
    }
}

   | SUB UnaryExp {
    SymbolEntry *se;

// 检查当前表达式是否为常量
if ($2->getSymbolEntry()->isConstant()) {
    // 如果是常量，进行常量折叠
    ConstantSymbolEntry* constEntry = dynamic_cast<ConstantSymbolEntry*>($2->getSymbolEntry());

    if (constEntry) {
        if (constEntry->getType()->isInt()) {
            int a = -constEntry->getValue();
            constEntry->setValue(a);
        }
        $$ = $2; // 使用更新后的常量表达式作为结果
    } else {
        // 常量转换失败，抛出错误或进行其他处理
        $$ = nullptr; // 错误处理，返回空指针
    }
} else if ($2->getSymbolEntry()->isId()) {
    // 如果是标识符，进行取负操作
    IdentifierSymbolEntry* idEntry = dynamic_cast<IdentifierSymbolEntry*>($2->getSymbolEntry());

    if (idEntry) {
        if (idEntry->getType()->isInt()) {
            int a = -idEntry->getValue();
            idEntry->setValue(a);
            //std::cout << "Updated value after negation: " << idEntry->getValue() << std::endl; 
        }
        $$ = $2; // 使用更新后的标识符作为结果
    } else {
        // 标识符转换失败，抛出错误或进行其他处理
        $$ = nullptr; // 错误处理，返回空指针
    }
} else {
    // 如果不是常量也不是标识符，则需要创建一个新的临时符号项
    se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
    $$ = new UnaryExpr(se, UnaryExpr::NEG, $2);
}
}
    | NOT UnaryExp {

    //cout << "NOT" << endl;
            Type* type;
        type = TypeSystem::boolType;
        SymbolEntry *se = new TemporarySymbolEntry(type, SymbolTable::getLabel());
        $$ = new UnaryExpr(se, UnaryExpr::NOT, $2);
    }
;
MulExp
    :
    UnaryExp {$$ = $1;}
    | MulExp MUL UnaryExp {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::MUL, $1, $3);
    }
    | MulExp DIV UnaryExp {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::DIV, $1, $3);
    }
    | MulExp MOD UnaryExp {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::MOD, $1, $3);
    }
    ;
RelExp
    : 
    AddExp { $$ = $1; }
    | RelExp EQUAL AddExp {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::EQUAL, $1, $3);
    }
    | RelExp NOTEQUAL AddExp {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::NOTEQUAL, $1, $3);
    }
    | RelExp LESS AddExp {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::LESS, $1, $3);
    }
    | RelExp LESSEQUAL AddExp {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::LESSEQUAL, $1, $3);
    }
    | RelExp GREATER AddExp {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::GREATER, $1, $3);
    }
    | RelExp GREATEREQUAL AddExp {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::GREATEREQUAL, $1, $3);
    }
    ;
LAndExp
    :
    RelExp {$$ = $1;}
    |
    LAndExp AND RelExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::AND, $1, $3);
    }
    ;
LOrExp
    :
    LAndExp {$$ = $1;}
    | LOrExp OR LAndExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::OR, $1, $3);
        
    }
    ;
Type
    : INT {
        $$ = TypeSystem::intType;
    }
    | VOID {
        $$ = TypeSystem::voidType;
    }
    | BOOL{
        $$ = TypeSystem::boolType;
    }
    /* | CONST Type {
        $$ = new ConstType($2);
    } */
    | DECIMAL {
        $$ = TypeSystem::decimalType;
    }
    | FLOAT {
        $$ = TypeSystem::floatType;
    }
    ;

DeclStmt
    : Type DeclStmtNodes SEMICOLON {
        $$ = $2;
        DeclStmt* curr = (DeclStmt*)$$;
        while (curr != nullptr) {
            curr->getId()->getSymPtr()->setType($1);
            curr = (DeclStmt*)curr->getNext();
            
        }
    }
    ;

ConstDeclStmt
    : CONST Type DeclStmtNodes SEMICOLON {
        $$ = $3;
        DeclStmt* curr = (DeclStmt*)$$;
        while (curr != nullptr) {
            curr->getId()->getSymPtr()->setType($2);
            ((IdentifierSymbolEntry*)curr->getId()->getSymPtr())->setConst();
            curr = (DeclStmt*)curr->getNext();
        }
    }
    ;

DeclStmtNodes
    : DeclStmtNodes COMMA DeclStmtNode{
        //fprintf(stderr, "declstmtnodes\n");
        $$ = $1;
        $1->setNext($3);
    }
    | DeclStmtNode {$$ = $1;}
    ;

DeclStmtNode
    : ID {  // 不带初始化的声明
        // 检查变量是否已在当前作用域声明
        if (identifiers->checkExist($1)) {
            fprintf(stderr, "类型检查错误：重定义变量 %s\n", $1);
            exit(EXIT_FAILURE);
        }

        // 默认类型为 int，如有其他类型需求，可在上层规则中设置
        SymbolEntry* se = new IdentifierSymbolEntry(TypeSystem::intType, $1, identifiers->getLevel());
        identifiers->install($1, se);

        // 创建声明语句节点
        $$ = new DeclStmt(new Id(se));
        delete[] $1;
    }
    | ID ASSIGN Exp {  // 带初始化的声明
        // 检查变量是否已在当前作用域声明
        if (identifiers->checkExist($1)) {
            fprintf(stderr, "类型检查错误：重定义变量 %s\n", $1);
            exit(EXIT_FAILURE);
        }
        if ($3->getType()->isVoid()){fprintf(stderr, "类型检查错误：%s 赋值为void\n", $1);
            exit(EXIT_FAILURE);};
        // 默认类型为 int，如有其他类型需求，可在上层规则中设置
        SymbolEntry* se = new IdentifierSymbolEntry(TypeSystem::intType, $1, identifiers->getLevel());
        identifiers->install($1, se);

        // 创建声明和赋值语句节点
        $$ = new DeclStmt(new Id(se), $3);
        delete[] $1;
    }
    ;


//reference
FuncDef
    :
    Type ID LPAREN {
        // 函数类型
        Type *funcType;
        funcType = new FunctionType($1, vector<Type*>());

        // 创建符号条目，并将函数名加入符号表
        SymbolEntry *se = new IdentifierSymbolEntry(funcType, $2, identifiers->getLevel());
        identifiers->install($2, se);

        // 创建新的符号表用于函数体的局部变量
        identifiers = new SymbolTable(identifiers);
    }
    FuncDefParams RPAREN {
        identifiers = new SymbolTable(identifiers);
        DeclStmt* curr = (DeclStmt*)$5;
        while(curr != nullptr)
        {
            std::string name = ((IdentifierSymbolEntry*)curr->getId()->getSymPtr())->getName();
            SymbolEntry *se = new IdentifierSymbolEntry(curr->getId()->getSymPtr()->getType(), name, identifiers->getLevel());
            identifiers->install(name, se);
            curr = (DeclStmt*)(curr->getNext());
        }
    }
    funcBlock {
        vector<Type*> paramsType;
        DeclStmt* curr = (DeclStmt*)$5;
        DeclStmt* paramDecl = nullptr;
        while(curr != nullptr)
        {
            if(paramDecl == nullptr)
            {
                paramDecl = new DeclStmt(new Id(identifiers->lookup(((IdentifierSymbolEntry*)curr->getId()->getSymPtr())->getName())), curr->getId());
                paramDecl->setIsParam();
            }
            else
            {
                DeclStmt* newDecl = new DeclStmt(new Id(identifiers->lookup(((IdentifierSymbolEntry*)curr->getId()->getSymPtr())->getName())), curr->getId());
                newDecl->setIsParam();
                paramDecl->setNext(newDecl);
            }
            paramsType.push_back(curr->getId()->getSymPtr()->getType());
            curr = (DeclStmt*)(curr->getNext());
        }
        
        // 查找函数符号，确保函数已声明
        SymbolEntry *se;
        se = identifiers->lookup($2);
        assert(se != nullptr);
                
        // 获取函数类型，并设置参数类型
        FunctionType* tmp = (FunctionType*)(se->getType());
        tmp->setParamsType(paramsType);
        if(paramDecl)
            ((SeqNode*)$8)->addStmtFront(paramDecl);

        // 创建函数定义对象，绑定符号条目和代码块
        $$ = new FunctionDef(se,new CompoundStmt($8), (DeclStmt*)$5);

                // 恢复符号表，退出函数体的作用域
        SymbolTable *top = identifiers;
        identifiers = identifiers->getPrev();
        delete top;
                
        // 释放函数名的内存
        delete []$2;
    }
    ;
funcBlock 
    : LBRACE Stmts RBRACE {
        $$ = $2;
    }
    | LBRACE RBRACE {
        $$ = new EmptyStmt();
    }
    ;
FuncDefParams
    : %empty { $$ = nullptr; }
    | FuncDefParam {
        $$ = $1;
    }
    | FuncDefParams COMMA FuncDefParam {
        $$ = $1;
        $$->setNext($3);
    }
    ;
FuncDefParam
    : Type ID {
        SymbolEntry* se;
        se = new IdentifierSymbolEntry($1, $2, identifiers->getLevel());
        identifiers->install($2, se);
        $$ = new DeclStmt(new Id(se));
        delete []$2;
    }
    | Type ID ASSIGN Exp {
        SymbolEntry* se;
        se = new IdentifierSymbolEntry($1, $2, identifiers->getLevel());
        identifiers->install($2, se);
        $$ = new DeclStmt(new Id(se), $4);
        delete []$2;
    }
    ;

FuncCallExp
    : ID LPAREN ExpList RPAREN {
        SymbolEntry *se;
        se = identifiers->lookup($1);
        if(se == nullptr)
            {
                fprintf(stderr, "类型检查错误：函数 \"%s\" 未定义\n", (char*)$1);
                exit(EXIT_FAILURE);
                delete [](char*)$1;
                assert(se != nullptr);
            }
    assert(se != nullptr);
        SymbolEntry* res = new TemporarySymbolEntry(((FunctionType*)se->getType())->getRetType(), SymbolTable::getLabel());
        $$ = new FuncCallExp(se, res, $3);
    }
    ;

%%

int yyerror(char const* message)
{
    std::cerr<<message<<std::endl;
    return -1;
}
