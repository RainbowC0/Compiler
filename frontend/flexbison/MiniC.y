%{
#include <cstdio>
#include <cstring>

// 词法分析头文件
#include "FlexLexer.h"

// bison生成的头文件
#include "BisonParser.h"

// 抽象语法树函数定义原型头文件
#include "AST.h"

#include "IntegerType.h"

#include "ArrayType.h"

#include <vector>

// LR分析失败时所调用函数的原型声明
void yyerror(char * msg);
ast_node* adjustCond(ast_node * node);
void adjustInit(const ArrayType *type, ast_node *node);
%}

// 联合体声明，用于后续终结符和非终结符号属性指定使用

%union {
    class ast_node * node;
    struct digit_int_attr integer_num;
    struct digit_real_attr float_num;
    struct var_id_attr var_id;
    struct type_attr type;
    int op_type;
    std::vector<uint32_t> * dims;
};

// 文法的开始符号
%start  CompileUnit

// 指定文法的终结符号，<>可指定文法属性
// 对于单个字符的算符或者分隔符，在词法分析时可直返返回对应的ASCII码值，bison预留了255以内的值
// %token开始的符号称之为终结符，需要词法分析工具如flex识别后返回
// %type开始的符号称之为非终结符，需要通过文法产生式来定义
// %token或%type之后的<>括住的内容成为文法符号的属性，定义在前面的%union中的成员名字。
%token <integer_num> T_DIGIT
%token <float_num> T_FLOAT_LITERAL
%token <var_id> T_ID
%token <type> T_INT
%token <type> T_FLOAT
%token <type> T_VOID
%token STRING_LITERAL
%token CONST

// 关键或保留字 一词一类 不需要赋予语义属性
%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN
%left mn
%left ELSE

// 分隔符 一词一类 不需要赋予语义属性
%token '{' '}'
%token ','

// 运算符
%left ';'
%right '=' T_ASSDIV T_ASSMUL T_ASSADD T_ASSSUB
%left T_LOR
%left T_LAND
%left <op_type> '|'
%left <op_type> '^'
%left <op_type> '&'
%left <op_type> T_EQ T_NE
%left <op_type> '>' T_GE '<' T_LE
%left <op_type> T_SL T_SR
%left <op_type> '+' '-'
%left <op_type> '*' '/' '%'
%left <op_type> '!'
%left '(' ')'

// 非终结符
// %type指定文法的非终结符号，<>可指定文法属性
%type <node> CompileUnit
%type <node> FuncDef
%type <node> Block
%type <node> BlockItemList
%type <node> BlockItem
%type <node> Statement
%type <node> Expr
%type <node> LVal
%type <node> VarDecl VarDeclExpr VarDef
%type <node> CondExp AndExp EqExp RelExp MulExp AddExp UnaryExp PrimaryExp
%type <node> RealParamList
%type <type> BasicType
//%type <type> FuncType
%type <node> IfStmt
%type <node> For
%type <node> While
%type <node> DoWhile
%type <node> FuncParam
%type <node> FuncParams
%type <op_type> AddOp
%type <op_type> MulOp
%type <op_type> RelOp
%type <node> ArrayIndexList ArrayInitList InitValueList InitValue
%%

// 编译单元可包含若干个函数与全局变量定义。要在语义分析时检查main函数存在
// compileUnit: (funcDef | varDecl)* EOF;
// bison不支持闭包运算，为便于追加修改成左递归方式
// compileUnit: funcDef | varDecl | compileUnit funcDef | compileUnit varDecl
CompileUnit : FuncDef {

		// 创建一个编译单元的节点AST_OP_COMPILE_UNIT
		$$ = create_contain_node(ASTOP(COMPILE_UNIT), $1);

		// 设置到全局变量中
		ast_root = $$;
	}
	| VarDecl {

		// 创建一个编译单元的节点AST_OP_COMPILE_UNIT
		$$ = create_contain_node(ASTOP(COMPILE_UNIT), $1);
		ast_root = $$;
	}
	| CompileUnit FuncDef {

		// 把函数定义的节点作为编译单元的孩子
		$$ = $1->insert_son_node($2);
	}
	| CompileUnit VarDecl {
		// 把变量定义的节点作为编译单元的孩子
		$$ = $1->insert_son_node($2);
	}
	;

// 函数定义，目前支持整数返回类型，不支持形参
FuncDef : BasicType T_ID '(' ')' Block {
		// 函数返回类型
		type_attr funcReturnType = $1;

		// 函数名
		var_id_attr funcId = $2;

		// 函数体节点即Block，即$5
		ast_node * blockNode = $5;

		// 形参结点没有，设置为空指针
		ast_node * formalParamsNode = nullptr;

		// 创建函数定义的节点，孩子有类型，函数名，语句块和形参(实际上无)
		// create_func_def函数内会释放funcId中指向的标识符空间，切记，之后不要再释放，之前一定要是通过strdup函数或者malloc分配的空间
		$$ = create_func_def(funcReturnType, funcId, blockNode, formalParamsNode);
	}
	| BasicType T_ID '(' FuncParams ')' Block  {
        $$ = create_func_def($1, $2, $6, $4);
	}
	;

FuncParams : FuncParam {
        $$ = create_contain_node(ASTOP(FUNC_FORMAL_PARAMS), $1);
	}
	| FuncParams ',' FuncParam {
        $$->insert_son_node($3);
	};

FuncParam : BasicType T_ID {
        $$ = ast_node::New($2);
        $$->type = typeAttr2Type($1);
	}
    | BasicType T_ID ArrayIndexList {
        $$ = ast_node::New($2);
        Type *tp = typeAttr2Type($1);
        
        // 计算数组维度，
        $$->type = tp;
        $$->insert_son_node($3);
    };

// 语句块的文法Block ： '{' BlockItemList? '}'
// 其中?代表可有可无，在bison中不支持，需要拆分成两个产生式
// Block ： '{' '}' | '{' BlockItemList '}'
Block : '{' '}' {
		// 语句块没有语句

		// 为了方便创建一个空的Block节点
		$$ = create_contain_node(ASTOP(BLOCK));
	}
	| '{' BlockItemList '}' {
		// 语句块含有语句

		// BlockItemList归约时内部创建Block节点，并把语句加入，这里不创建Block节点
		$$ = $2;
	}
	;

// 语句块内语句列表的文法：BlockItemList : BlockItem+
// Bison不支持正闭包，需修改成左递归形式，便于属性的传递与孩子节点的追加
// 左递归形式的文法为：BlockItemList : BlockItem | BlockItemList BlockItem
BlockItemList : BlockItem {
		// 第一个左侧的孩子节点归约成Block节点，后续语句可持续作为孩子追加到Block节点中
		// 创建一个AST_OP_BLOCK类型的中间节点，孩子为Statement($1)
		$$ = create_contain_node(ASTOP(BLOCK), $1);
	}
	| BlockItemList BlockItem {
		// 把BlockItem归约的节点加入到BlockItemList的节点中
		$$ = $1->insert_son_node($2);
	}
	;


// 语句块中子项的文法：BlockItem : Statement
// 目前只支持语句,后续可增加支持变量定义
BlockItem : Statement  {
		// 语句节点传递给归约后的节点上，综合属性
		$$ = $1;
	}
	| VarDecl {
		// 变量声明节点传递给归约后的节点上，综合属性
		$$ = $1;
	}
	;

// 变量声明语句
// 语法：varDecl: basicType varDef (',' varDef)* ';'
// 因Bison不支持闭包运算符，因此需要修改成左递归，修改后的文法为：
// VarDecl : VarDeclExpr ';'
// VarDeclExpr: BasicType VarDef | VarDeclExpr ',' varDef
VarDecl : VarDeclExpr ';' {
		$$ = $1;
	}
	;

// 变量声明表达式，可支持逗号分隔定义多个
VarDeclExpr: BasicType VarDef {
        $2->type = typeAttr2Type($1);
		// 创建变量声明语句，并加入第一个变量
		$$ = create_contain_node(ASTOP(VAR_DECL), $2);
	}
    | CONST BasicType VarDef {
        $3->type = typeAttr2Type($2);
        $3->isConst = true;
        $$ = create_contain_node(ASTOP(VAR_DECL), $3);
    }
	| VarDeclExpr ',' VarDef {
		// 插入到变量声明语句
        ast_node * s0 = $1->sons[0];
        $3->type = s0->type;
        $3->isConst = s0->isConst;
		$$ = $1->insert_son_node($3);
	}
	;

// 变量定义包含变量名，实际上还有初值，这里没有实现。
VarDef : T_ID {
		// 简单变量定义ID

		// 创建变量ID终结符节点
		$$ = ast_node::New($1);

		// 对于字符型字面量的字符串空间需要释放
		free($1.id);
	}
	| T_ID ArrayIndexList {
		// 多维数组变量定义ID[expr1][expr2]...，支持常量表达式
        $$ = ast_node::New(var_id_attr{$1.id, $1.lineno});
        
        // 将ArrayIndexList作为子节点保存，用于后续计算维度
        $$->insert_son_node($2);

		// 对于字符型字面量的字符串空间需要释放
		free($1.id);
	}
	| T_ID '=' Expr {
        $$ = ast_node::New(var_id_attr{$1.id, $1.lineno});
        $$->insert_son_node($3);
        free($1.id);
    }
	| T_ID ArrayIndexList '=' ArrayInitList {
		// 多维数组带初始化定义，支持常量表达式
		$$ = ast_node::New(var_id_attr{$1.id, $1.lineno});
        
        // 将ArrayIndexList和初始化列表作为子节点保存
        $$->insert_son_node($2);
        $$->insert_son_node($4);
		free($1.id);
	}
	;

// 数组初始化列表：{1, 2, 3} 或 {{1, 2}, {3, 4}}
ArrayInitList : '{' InitValueList '}' {
		$$ = $2;
	}
	| '{' '}' {
		// 空初始化列表
		$$ = create_contain_node(ASTOP(ARRAY_INIT), nullptr);
	}
	;

// 初始化值列表
InitValueList : InitValue {
		$$ = create_contain_node(ASTOP(ARRAY_INIT), $1);
	}
	| InitValueList ',' InitValue {
		$$ = $1->insert_son_node($3);
	}
	;

// 初始化值：可以是表达式或者嵌套的初始化列表
InitValue : Expr {
		$$ = $1;
	}
	| ArrayInitList {
		$$ = $1;
	}
	;

// 基本类型，目前只支持整型
BasicType: T_INT { $$ = $1; }
	| T_FLOAT { $$ = $1; }
	| T_VOID { $$ = $1; }
	;

// 语句文法：statement:RETURN expr ';' | lVal '=' expr ';'
// | block | expr? ';'
// 支持返回语句、赋值语句、语句块、表达式语句
// 其中表达式语句可支持空语句，由于bison不支持?，修改成两条
Statement : RETURN Expr ';' {
		// 返回语句

		// 创建返回节点AST_OP_RETURN，其孩子为Expr，即$2
		$$ = create_contain_node(ASTOP(RETURN), $2);
	}
    | RETURN ';' {
        $$ = new ast_node(ASTOP(RETURN));
    }
	| LVal '=' Expr ';' {
		// 赋值语句

		// 创建一个AST_OP_ASSIGN类型的中间节点，孩子为LVal($1)和Expr($3)
		$$ = create_contain_node(ASTOP(ASSIGN), $1, $3);
	}
	| Block {
		// 语句块
		// 内部已创建block节点，直接传递给Statement
		$$ = $1;
	}
	| Expr ';' {
		// 表达式语句
		// 内部已创建表达式，直接传递给Statement
		$$ = $1;
	}
	| IfStmt {
		$$ = $1;
	}
	| While {
		$$ = $1;
	}
	| DoWhile {
		$$ = $1;
	}
	| For {
		$$ = $1;
	}
	| BREAK ';' {
		$$ = new ast_node(ASTOP(BREAK));
	}
	| CONTINUE ';' {
		$$ = new ast_node(ASTOP(CONTINUE));
	}
	| ';' {
		// 空语句

		// 直接返回空指针，需要再把语句加入到语句块时要注意判断，空语句不要加入
		$$ = new ast_node(ASTOP(NULL_STMT));
	}
	;

// 表达式文法 expr : AddExp
// 表达式目前只支持加法与减法运算
Expr : AddExp {
		// 直接传递给归约后的节点
		$$ = $1;
	}
	;

// 加减表达式文法：addExp: unaryExp (addOp unaryExp)*
// 由于bison不支持用闭包表达，因此需要拆分成左递归的形式
// 改造后的左递归文法：
// addExp : unaryExp | unaryExp addOp unaryExp | addExp addOp unaryExp
AddExp : MulExp {
		// 一目表达式
		// 直接传递到归约后的节点
		$$ = $1;
	}
    | AddExp AddOp MulExp { // TODO 隐式转换
		// 创建加减运算节点，孩子为AddExp($1)和UnaryExp($3)
        bool li = $1->node_type == ASTOP(LEAF_LITERAL_INT);
        bool lf = $1->node_type == ASTOP(LEAF_LITERAL_FLOAT);
        bool ri = $3->node_type == ASTOP(LEAF_LITERAL_INT);
        bool rf = $3->node_type == ASTOP(LEAF_LITERAL_FLOAT);
        if (li && rf) {
            lf = true; li = false;
            $1->float_val = $1->integer_val;
            $1->node_type = ASTOP(LEAF_LITERAL_FLOAT);
        } else if (lf && ri) {
            rf = true; ri = false;
            $3->float_val = $3->integer_val;
            $3->node_type = ASTOP(LEAF_LITERAL_FLOAT);
        }
        if (li && ri) {
            if ($2 == (int)ASTOP(ADD))
                $1->integer_val += $3->integer_val;
            else
                $1->integer_val -= $3->integer_val;
            $$ = $1;
            delete $3;
        } else if (lf && rf) {
            if ($2 == (int)ASTOP(ADD))
                $1->float_val += $3->float_val;
            else
                $1->float_val -= $3->float_val;
            $$ = $1;
            delete $3;
        } else {
		    $$ = create_contain_node(ast_operator_type($2), $1, $3);
        }
	}
	;

AddOp: '+' { $$ = (int)ASTOP(ADD); }
	| '-' { $$ = (int)ASTOP(SUB); }
    ;

MulExp: UnaryExp { $$ = $1; }
    | MulExp MulOp UnaryExp {
    bool li = $1->node_type == ASTOP(LEAF_LITERAL_INT);
        bool lf = $1->node_type == ASTOP(LEAF_LITERAL_FLOAT);
        bool ri = $3->node_type == ASTOP(LEAF_LITERAL_INT);
        bool rf = $3->node_type == ASTOP(LEAF_LITERAL_FLOAT);
        if (li && rf) {
            lf = true; li = false;
            $1->float_val = $1->integer_val;
            $1->node_type = ASTOP(LEAF_LITERAL_FLOAT);
        } else if (lf && ri) {
            rf = true; ri = false;
            $3->float_val = $3->integer_val;
            $3->node_type = ASTOP(LEAF_LITERAL_FLOAT);
        }
        if (li && ri) {
            switch (ast_operator_type($2)) {
                case ASTOP(MUL): $1->integer_val *= $3->integer_val; break;
                case ASTOP(DIV): $1->integer_val /= $3->integer_val; break;
                default: $1->integer_val %= $3->integer_val; break;
            }
            $$ = $1;
            delete $3;
        } else if (lf && rf) {
            switch (ast_operator_type($2)) {
                case ASTOP(MUL): $1->float_val *= $3->float_val; break;
                case ASTOP(DIV): $1->float_val /= $3->float_val; break;
                default: break;
            }
            $$ = $1;
            delete $3;
        } else {
            if ($2 == (int)ASTOP(MUL) && ((li && $1->integer_val == 1) || (lf && $1->float_val == 1.f))) {
                $$ = $3;
                delete $1;
            } else if ((ri && $3->integer_val == 1) || (rf && $3->float_val == 1.f)) {
                $$ = $1;
                delete $3;
            } else
                $$ = create_contain_node(ast_operator_type($2), $1, $3);
        }
    }
    ;

MulOp: '*' { $$ = (int)ASTOP(MUL); }
	| '/' { $$ = (int)ASTOP(DIV); }
	| '%' { $$ = (int)ASTOP(MOD); }
	;

RelExp:
	AddExp {
		$$ = $1;
	}
	| RelExp RelOp AddExp {
		$$ = create_contain_node(ast_operator_type($2), $1, $3);
	}
	;

RelOp: '>' { $$ = (int)ASTOP(GT); }
	| T_GE { $$ = (int)ASTOP(GE); }
	| '<' { $$ = (int)ASTOP(LT); }
	| T_LE { $$ = (int)ASTOP(LE); }
	;

EqExp: RelExp { $$ = $1; }
    | EqExp T_EQ RelExp { $$ = create_contain_node(ASTOP(EQ), $1, $3); }
    | EqExp T_NE RelExp { $$ = create_contain_node(ASTOP(NE), $1, $3); }
    ;

AndExp: EqExp { $$ = adjustCond($1); }
    | AndExp T_LAND EqExp {
        $$ = create_contain_node(ASTOP(LAND), $1, adjustCond($3));
    }
    ;

// OrExp
CondExp:
	AndExp { $$ = $1; }
	| CondExp T_LOR AndExp {
		$$ = create_contain_node(ASTOP(LOR), $1, $3);
	}
    ;

// 目前一元表达式可以为基本表达式、函数调用，其中函数调用的实参可有可无
// 其文法为：unaryExp: primaryExp | T_ID '(' realParamList? ')'
// 由于bison不支持？表达，因此变更后的文法为：
// unaryExp: primaryExp | T_ID '(' ')' | T_ID '(' realParamList ')'
UnaryExp : PrimaryExp {
		// 基本表达式

		// 传递到归约后的UnaryExp上
		$$ = $1;
	}
	| T_ID '(' ')' {
		// 没有实参的函数调用

		// 创建函数调用名终结符节点
		ast_node * name_node = ast_node::New(std::string($1.id), $1.lineno);

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);

		// 实参列表
		ast_node * paramListNode = nullptr;

		// 创建函数调用节点，其孩子为被调用函数名和实参，实参为空，但函数内部会创建实参列表节点，无孩子
		$$ = create_func_call(name_node, paramListNode);

	}
	| T_ID '(' RealParamList ')' {
		// 含有实参的函数调用

		// 创建函数调用名终结符节点
		ast_node * name_node = ast_node::New(std::string($1.id), $1.lineno);

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);

		// 实参列表
		ast_node * paramListNode = $3;

		// 创建函数调用节点，其孩子为被调用函数名和实参，实参不为空
		$$ = create_func_call(name_node, paramListNode);
	}
	| '+' UnaryExp { $$ = $2; }
	| '-' UnaryExp {
		// TODO 浮点数
        if ($2->node_type==ASTOP(LEAF_LITERAL_INT)) {
            $2->integer_val = -$2->integer_val;
            $$ = $2;
        } else if ($2->node_type==ASTOP(LEAF_LITERAL_FLOAT)) {
            $2->float_val = -$2->float_val;
            $$ = $2;
        } else {
            auto v = ast_node::New(digit_int_attr{0,$2->line_no});
            $$ = create_contain_node(ASTOP(SUB), v, $2);
        }
	}
	| '!' UnaryExp {
		$$ = create_contain_node(ASTOP(NOT), $2);
	}
	;

// 基本表达式支持无符号整型字面量、带括号的表达式、具有左值属性的表达式
// 其文法为：primaryExp: '(' expr ')' | T_DIGIT | lVal
PrimaryExp :  '(' Expr ')' {
		// 带有括号的表达式
		$$ = $2;
	}
	| T_DIGIT {
        	// 无符号整型字面量
		// 创建一个无符号整型的终结符节点
		$$ = ast_node::New($1);
	}
	| T_FLOAT_LITERAL {
		$$ = ast_node::New($1);
	}
	| LVal  {
		// 具有左值的表达式

		// 左转右，部分需要特殊处理
		$$ = create_contain_node(ASTOP(L2R), $1);
	}
	;

// 实参表达式支持逗号分隔的若干个表达式
// 其文法为：realParamList: expr (',' expr)*
// 由于Bison不支持闭包运算符表达，修改成左递归形式的文法
// 左递归文法为：RealParamList : Expr | 左递归文法为：RealParamList ',' expr
RealParamList : Expr {
		// 创建实参列表节点，并把当前的Expr节点加入
		$$ = create_contain_node(ast_operator_type::AST_OP_FUNC_REAL_PARAMS, $1);
	}
	| RealParamList ',' Expr {
		// 左递归增加实参表达式
		$$ = $1->insert_son_node($3);
	}
	;

// 左值表达式，目前只支持变量名或数组元素
LVal : T_ID {
		// 变量名终结符

		// 创建变量名终结符节点
		$$ = ast_node::New($1);

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);
	}
	| T_ID ArrayIndexList {
		// 多维数组访问 ID[expr1][expr2]...
		
		// 创建多维数组访问节点
		$$ = create_contain_node(ASTOP(ARRAY_ACCESS), ast_node::New($1), $2);
		
		// 对于字符型字面量的字符串空间需要释放
		free($1.id);
	}
	;

// 数组索引列表：[expr1][expr2][expr3]...
ArrayIndexList : '[' Expr ']' {
		// 创建数组索引列表节点，包含第一个索引
		$$ = create_contain_node(ASTOP(ARRAY_INDICES), $2);
	}
    | '[' ']' {
        $$ = create_contain_node(ASTOP(ARRAY_INDICES), ast_node::New(digit_int_attr{0, yylineno}));
    }
	| ArrayIndexList '[' Expr ']' {
		// 向数组索引列表添加新索引
		$$ = $1->insert_son_node($3);
	}
	;

IfStmt : IF '(' CondExp ')' Statement ELSE Statement {
		$$ = create_contain_node(ASTOP(IF), $3, $5, $7);
	}
	| IF '(' CondExp ')' Statement %prec mn {
		$$ = create_contain_node(ASTOP(IF), $3, $5);
	}
	;

While : WHILE '(' CondExp ')' Statement {
		$$ = create_contain_node(ast_operator_type::AST_OP_WHILE, $3, $5);
	}
	;

For : FOR '(' Expr ';' CondExp ';' Expr ')' Statement {
		//$$ = create_contain_node(ast_operator_type::AST_OP_FOR, $3, $5, $7, $9);
	}
	;

DoWhile : DO Statement WHILE '(' CondExp ')' ';' {
		$$ = create_contain_node(ast_operator_type::AST_OP_DOWHILE, $2, $5);
	}
	;
%%

// 语法识别错误要调用函数的定义
void yyerror(char * msg)
{
    printf("Line %d: %s\n", yylineno, msg);
}

// 转换条件 a -> a!=0，保证条件式内无NOT节点
ast_node* adjustCond(ast_node *node) {
    // TODO 浮点数
    auto x = node->node_type;
    if (ASTOP(EQ)<=x && x<=ASTOP(LE)) return node;
    auto v = ast_node::New(digit_int_attr{0,yylineno});
    int tp = (int)ASTOP(NE);
    while (node->node_type==ASTOP(NOT)) {
        tp = (((int)ASTOP(EQ))^((int)ASTOP(NE)))^tp;
        node = node->sons[0];
    }
    return create_contain_node(ast_operator_type(tp), node, v);
}

// init-list标准化调整
void adjustInit(const ArrayType *type, ast_node *node) {
    
}
