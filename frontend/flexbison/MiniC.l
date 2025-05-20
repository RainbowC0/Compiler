%{
/* 这里声明语义动作符程序所需要的函数原型或者变量原型或定义等 */
/* 主要包含头文件，extern的全局变量，定义的全局变量等 */

#include <cstdio>
#include <cstdint>
#include <cstring>

// 此文件定义了文法中终结符的类别
#include "BisonParser.h"

#define SETINT(p) yylval.integer_num.val = strtol(yytext, NULL, p);\
    yylval.integer_num.lineno = yylineno;\
    return T_DIGIT

// 对于整数或浮点数，词法识别无符号数，对于负数，识别为求负运算符与无符号数，请注意。
%}

/* 使它不要添加默认的规则,这样输入无法被给定的规则完全匹配时，词法分析器可以报告一个错误 */
%option nodefault

/* 产生yywrap函数 */
%option noyywrap

/* flex 生成的扫描器用全局变量yylineno 维护着输入文件的当前行编号 */
%option yylineno

/* 区分大小写 */
%option case-sensitive

/* yytext的类型为指针类型，即char * */
%option pointer

/* 生成可重用的扫描器API，这些API用于多线程环境 */
/* %option reentrant */

/* 不进行命令行交互，只能分析文件 */
%option never-interactive

/* 辅助定义式或者宏，后面使用时带上大括号 */
D           [0-9]
L           [a-zA-Z_]
H           [a-fA-F0-9]
E           ([Ee][+-]?{D}+)
P           ([Pp][+-]?{D}+)

/*|{TCMT}*/
SCMT        "//"[^\n]*
TCMT        "/*"([^*]|\*+[^*/])*(\*+"/")?
CMT         {SCMT}|{TCMT}

/* 正规式定义 */
%%

"("         { return '('; }
")"         { return ')'; }
"{"         { return '{'; }
"}"         { return '}'; }
"["         { return '['; }
"]"         { return ']'; }

";"         { return ';'; }
","         { return ','; }

"="         { return '='; }
"+"         { return '+'; }
"-"         { return '-'; }
"*"         { return '*'; }
"/"         { return '/'; }
"%"         { return '%'; }
"!"         { return '!'; }
"<<"        { return T_SL; }
">>"        { return T_SR; }
">"         { return '>'; }
"<"         { return '<'; }
">="        { return T_GE; }
"<="        { return T_LE; }
"=="        { return T_EQ; }
"!="        { return T_NE; }
"&"         { return '&'; }
"|"         { return '|'; }
"^"         { return '^'; }
"&&"        { return T_LAND; }
"||"        { return T_LOR; }
"/="        { return T_ASSDIV; }
"*="        { return T_ASSMUL; }
"+="        { return T_ASSADD; }
"-="        { return T_ASSSUB; }


[1-9]{D}*   { SETINT(10); }
0[0-7]*     { SETINT(8); }
0[xX]{H}+   { SETINT(16); }

"int"       {
                // int类型关键字 关键字的识别要在标识符识别的前边，这是因为关键字也是标识符，不过是保留的
                yylval.type.type = BasicType::TYPE_INT;
                yylval.type.lineno = yylineno;
                return T_INT;
            }
{D}+{E}|{D}*"."{D}+{E}?|{D}+"."{D}*{E}?|0[xX]{H}+{P}|0[xX]{H}*"."{H}+{P}?|0[xX]{H}+"."{H}*{P}? {
                yylval.float_num.val = strtof(yytext, NULL);
                yylval.float_num.lineno = yylineno;
                return T_FLOAT_LITERAL;
            }
"float"     {
                yylval.type.type = BasicType::TYPE_FLOAT;
                yylval.type.lineno = yylineno;
                return T_FLOAT;
            }
"return"    {
                // return关键字 关键字的识别要在标识符识别的前边，，这是因为关键字也是标识符，不过是保留的
                return RETURN;
            }
"case"      { return CASE; }
"default"   { return DEFAULT; }
"if"        { return IF; }
"else"      { return ELSE; }
"switch"    { return SWITCH; }
"while"     { return WHILE; }
"do"        { return DO; }
"for"       { return FOR; }
"goto"      { return GOTO; }
"continue"  { return CONTINUE; }
"break"     { return BREAK; }

{L}({L}|{D})* {
                // strdup 分配的空间需要在使用完毕后使用free手动释放，否则会造成内存泄漏
                yylval.var_id.id = strdup(yytext);
                yylval.var_id.lineno = yylineno;
                return T_ID;
            }

L?\"(\\.|[^\\"\n])*\"   { return STRING_LITERAL; }

[ \t\v\f\r\n]+     {
                // 空白符忽略
                ;
            }
{CMT}       { /* comment */ }

.           {
                printf("Line %d: Invalid char %s\n", yylineno, yytext);
                // 词法识别错误
                return 257;
            }
%%
