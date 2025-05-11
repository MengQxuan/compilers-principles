%option noyywrap
%option nounput
%option noinput
%top{
    #include <stdarg.h>
    #include "common.h"
    #include "parser.h"
}
%{
    extern dump_type_t dump_type;

    inline void dump_tokens(const char* format, ...){
        va_list args;
        va_start(args, format);
        if (dump_type == TOKENS)
            vfprintf(yyout, format, args);
        va_end(args);
    }

    /* Your code here, if desired (lab3). */
%}

/* definitions section */
DECIMIAL ([1-9][0-9]*|0)
ID [[:alpha:]_][[:alpha:][:digit:]_]*
EOL (\r\n|\n|\r)
WHITE [\t ]
OCTAL 0[0-7]+
HEXADECIMAL 0[xX][0-9a-fA-F]+
FLOAT ([0-9]+\.[0-9]*([eE][+-]?[0-9]+)?|[0-9]*\.[0-9]+([eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+)
LINE_COMMENT "//".*
BLOCK_COMMENT "/*"([^*]|\*+[^*/])*"*/"
OCTAL_FLOAT 0([0-7]+\.[0-7]*|[0-7]*\.[0-7]+)([pP][+-]?[0-9]+)? 
HEX_FLOAT 0[xX]([0-9a-fA-F]+(\.[0-9a-fA-F]*)?|\.[0-9a-fA-F]+)([pP][+-]?[0-9]+)?
BOOL (true|false)

/*  Your code here (lab3). */

%%
    /* rules section */
"int" {
    dump_tokens("INT\t%s\n", yytext);
    return INT;
}

"float" {
    dump_tokens("FLOAT\t%s\n", yytext);
    return FLOAT;
}

"bool" {
    dump_tokens("INT\t%s\n", yytext);
    return BOOL;
}

"decimal" {
    dump_tokens("DECIMAL\t%s\n", yytext);
    return DECIMAL;
}

"octal" {
    dump_tokens("OCTAL\t%s\n", yytext);
    return OCTAL;
}

"hexadecimal" {
    dump_tokens("HEXADECIMAL\t%s\n", yytext);
    return HEXADECIMAL;
}

"octal_float" {
    dump_tokens("OCTAL_FLOAT\t%s\n", yytext);
    return OCTAL_FLOAT;
}

"hex_float" {
    dump_tokens("HEX_FLOAT\t%s\n", yytext);
    return HEX_FLOAT;
}

"const" {
    dump_tokens("CONST\t%s\n", yytext);
    return CONST;
}

"void" {
    dump_tokens("VOID\t%s\n", yytext);
    return VOID;
}

"if" {
    dump_tokens("IF\t%s\n", yytext);
    return IF;
}

"else" {
    dump_tokens("ELSE\t%s\n", yytext);
    return ELSE;
}

"while" {
    dump_tokens("WHILE\t%s\n", yytext);
    return WHILE;
}

"return" {
    dump_tokens("RETURN\t%s\n", yytext);
    return RETURN;
}

"=" {
    dump_tokens("ASSIGN\t%s\n", yytext);
    return ASSIGN;
}

"!=" {
    dump_tokens("NOTEQUAL\t%s\n", yytext);
    return NOTEQUAL;
}

"==" {
    dump_tokens("EQUAL\t%s\n", yytext);
    return EQUAL;
}

"<" {
    dump_tokens("LESS\t%s\n", yytext);
    return LESS;
}

"<=" {
    dump_tokens("LESSEQUAL\t%s\n", yytext);
    return LESSEQUAL;
}

">" {
    dump_tokens("GREATER", yytext);
    return GREATER;
}

">=" {
    dump_tokens("GREATEREQUAL", yytext);
    return GREATEREQUAL;
}

"+" {
    dump_tokens("ADD\t%s\n", yytext);
    return ADD;
}

"-" {
    dump_tokens("SUB\t%s\n", yytext);
    return SUB;
}

"*" {
    dump_tokens("MUL\t%s\n", yytext);
    return MUL;
}

"/" {
    dump_tokens("DIV\t%s\n", yytext);
    return DIV;
}

"%" {
    dump_tokens("MOD\t%s\n", yytext);
    return MOD;
}

"!" {
    dump_tokens("NOT\t%s\n", yytext);
    return NOT;
}

"&&" {
    dump_tokens("AND\t%s\n", yytext);
    return AND;
}

"||" {
    dump_tokens("OR\t%s\n", yytext);
    return OR;
}

"break" {
    dump_tokens("BREAK\t%s\n", yytext);
    return BREAK;
}

"continue" {
    dump_tokens("CONTINUE\t%s\n", yytext);
    return CONTINUE;
}

"," {
    dump_tokens("COMMA\t%s\n", yytext);
    return COMMA;
}

";" {
    dump_tokens("SEMICOLON\t%s\n", yytext);
    return SEMICOLON;
}

"(" {
    dump_tokens("LPAREN\t%s\n", yytext);
    return LPAREN;
}

")" {
    dump_tokens("RPAREN\t%s\n", yytext);
    return RPAREN;
}

"{" {
    dump_tokens("LBRACE\t%s\n", yytext);
    return LBRACE;
}

"}" {
    dump_tokens("RBRACE\t%s\n", yytext);
    return RBRACE;
}
{FLOAT} {
    double float_value; // 使用 double 存储浮点数
    float_value = atof(yytext); // 将 yytext 转换为浮点数
    dump_tokens("FLOAT\t%s\t%f\n", yytext, float_value); // 输出识别到的浮点数
    yylval.ftype = float_value; // 将浮点数赋值给 yylval
    return FLOAT_LITERAL; 
}
{BOOL} {
    bool bool_value;
    bool_value = (strcmp(yytext, "true") == 0); // 通过比较字符串来设置布尔值
    dump_tokens("BOOL\t%s\t%b\n", yytext, bool_value); // 打印布尔值，使用 %b 格式符来输出布尔值
    yylval.btype = bool_value; // 将布尔值赋给 yylval
    return BOOL_LITERAL;
}

{DECIMIAL} {
    int decimal;
    decimal = atoi(yytext);
    dump_tokens("DECIMAL\t%s\t%d\n", yytext, decimal);
    yylval.itype = decimal;
    return INTEGER;
}

{OCTAL} {
    int octal_value;
    octal_value = strtol(yytext, nullptr, 8);
    dump_tokens("OCTAL\t%s\t%d\n", yytext, octal_value);
    yylval.itype = octal_value;
    return OCTAL_LITERAL;
}
{HEXADECIMAL} {
    int hex_value;
    hex_value = strtol(yytext, nullptr, 16);
    dump_tokens("HEXADECIMAL\t%s\t%d\n", yytext, hex_value);
    yylval.itype = hex_value;
    return HEX_LITERAL;
}

{ID} {
    char *lexeme;
    dump_tokens("ID\t%s\n", yytext);
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    return ID;
}

{EOL} yylineno++;

{LINE_COMMENT} {
    // 跳过行注释
}

{BLOCK_COMMENT} {
    // 在匹配块注释时，使用字符串处理逐行扫描
    const char* p = yytext; // yytext 是匹配到的块注释
    while (*p) {
        if (*p == '\n') {
            yylineno += 1; // 增加行号
            //colno = 1;     // 重置列号
        }
        p++;
    }
    // 跳过块注释
}


{WHITE}

    /*  Your code here (lab3). */
%%
/* user code section */
