#ifndef PARSER_H
#define PARSER_H

#include "dependencies.h"
#include "scanner.h"
/*
STMT_LET
PTR_TO_STR
TYPE
PTR_TO_EXPR
*/
#define ARENA_NULL 0
#define arrfree_and_set_null(op) do { arrfree(op); op = NULL; } while(0)

typedef enum
{
    STMT_ERR              ,
    STMT_LET_BARE         ,
    STMT_LET_TYPE         ,
    STMT_LET_EXPR         ,
    STMT_LET_TYPE_AND_EXPR,
    STMT_EXPR             ,
    STMT_IF               ,
    STMT_ELIF             ,
    STMT_ELSE             ,
    STMT_WHILE            ,
    STMT_BREAK            ,
    STMT_CONTINUE         ,
    STMT_FN               ,
    STMT_RETURN           ,
    STMT_EMPTY            ,
}
StmtType;

typedef enum
{
    // Atom
    OP_STRING,
    OP_IDENTIFIER,
    OP_INTEGER,
    OP_NUMBER,
    OP_TRUE,
    OP_FALSE,
    OP_NIL,
    OP_PRINT,
    // Prefix
    OP_NEG, // unary '-'
    OP_NOT,
    OP_PRE_INC,
    OP_PRE_DEC,
    // Postfix
    OP_INDEX,
    OP_POST_INC,
    OP_POST_DEC,
    // Infix
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_AND,
    OP_OR ,
    OP_GREATER,
    OP_LESS,
    OP_GREATER_EQUAL,
    OP_LESS_EQUAL,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_ACCESS,
    OP_CHAIN,
    // Assign (right associative)
    OP_ASSIGN,
    OP_ASSIGN_ADD,
    OP_ASSIGN_SUB,
    OP_ASSIGN_MUL,
    OP_ASSIGN_MOD,
    // Unique
    OP_CALL,
    //OP_COLON, // technically binary, but it has different behaviour
}
ExprOpType;

/*
*/
typedef enum
{
    TYPE_UNKNOWN,
    TYPE_NIL    ,
    TYPE_BOOL   ,
    TYPE_INT    , // i64 - for now at least
    TYPE_FLOAT  , // f64 - for now at least
}
ExprAtomicType;

typedef struct
{
    ExprOpType     op_type ;
    ExprAtomicType type    ;
    int            args    ;

    const char* literal;     // Points to parser literals (string, int, num, etc.)
    int line  ;
    int column;
    int length;
}
ExprOp;

typedef union
{
    struct
    {
        StmtType type ;
        int depth;
    }
    header;

    struct
    {
        int line  ;
        int column;
    }
    position;
    int length;

    ExprOp* expr;
    char  * literal;
}
Stmt;

typedef enum
{
    PARSER_STATE_UNPARSED,
    PARSER_STATE_PARSED  ,
}
ParserState;

typedef struct
{
    ParserState state ;
    const char* txt   ;

    Token*      tokens;
    int start;
    int end;
    int current;

    // All of these have to be free at a later date.
    ExprOp* exprs; // Not used right now
    char**  identifiers ;
    char**  str_literals; // Not used right now
    char**  int_literals; // Not used right now
    char**  num_literals; // Not used right now

    // If we fail to parse something for whatever reason, we append and error message here.
    char** log;
}
Parser;

Parser init_parser(Scanner scanner);
Parser free_parser(Scanner scanner);

Token parser_peek(Parser* parser);
Token parser_next(Parser* parser);

typedef enum
{
    LHS_OP_TYPE_ATOM  ,
    LHS_OP_TYPE_PREFIX,
    LHS_OP_TYPE_PARENS,
}
LhsOpType;

void stmt_append_header(Stmt** stmt, StmtType type, int depth, int line, int column, int length);
Stmt* parser_parse_stmts(Parser* parser, int depth);
Stmt* parser_parse_stmt(Parser* parser, int depth);
bool parser_skip(Parser* parser, bool (*predicate)(TokenType));
bool is_newline(TokenType type);
char* parser_parse_identifier(Parser* parser);
// size_t parser_parse_type(Parser* parser);
Stmt* parser_parse_block(Parser* parser, int depth);

ExprOp* parser_parse_expr(Parser* parser);
ExprOp* parser_parse_expr_inner(Parser* parser, int min_binding_power);

ExprOp* parser_parse_type(Parser* parser);
ExprOp* parser_parse_type_inner(Parser* parser, int min_binding_power);

void prefix_binding_power(ExprOpType op_type, int* right);
bool postfix_binding_power(ExprOpType op_type, int* left);
bool infix_binding_power(ExprOpType op_type, int* left, int* right);

void append_rhs_to_expr(ExprOp** expr, ExprOp** rhs);

//ExprOp* parser_parse_expr_inner(Parser parser, int minimum_binding_power);

//void infix_binding_power(ExprOpType op, int* left, int* right);
//void prefix_binding_power(ExprOp op, int* right);
void print_expr_op(ExprOp* op);


const char* stmt_type_name(StmtType type);
void print_stmt(Stmt* stmt);

#endif
