#include "parser.h"

Parser init_parser(Scanner scanner)
{
    Parser parser =
    {
        .state        = PARSER_STATE_UNPARSED     ,
        .txt          = scanner.init              ,
        .tokens       = scanner.token_list        ,
        .start        = 0                         ,
        .end          = arrlen(scanner.token_list),
        .current      = 0                         ,
        .exprs        = NULL                      ,
        .identifiers  = NULL                      ,
        .str_literals = NULL                      ,
        .int_literals = NULL                      ,
        .num_literals = NULL                      ,
        .log          = NULL                      ,
    };

    return parser;
}

Token parser_peek(Parser* parser)
{
    const char* err = "No more tokens left.";
    if (parser->current >= parser->end)
    {
        return (Token)
        {
            .type   = TOKEN_ERROR,
            .start  = err,
            .line   = -1,
            .column = -1,
            .length = strlen(err)
        };
    }
    return parser->tokens[parser->current];
}

Token parser_next(Parser* parser)
{
    Token token = parser_peek(parser);
    if (token.type != TOKEN_ERROR)
        parser->current++;
    return token;
}


bool parser_skip(Parser* parser, bool (*predicate)(TokenType))
{
    bool ret = false;
    while (true)
    {
        Token token = parser_peek(parser);
        if (predicate(token.type))
        {
            parser_next(parser);
            ret = true;
        }
        else
            break;
    }
    return ret;
}

bool is_newline(TokenType type)
{
    switch (type)
    {
        case TOKEN_SEMICOLON: return true;
        case TOKEN_NEWLINE  : return true;
        default: return false;
    }
}

void stmt_append_header(Stmt** stmt, StmtType type, int depth, int line, int column, int length)
{
    Stmt node;
    node = (Stmt){ .header = { .type = type, .depth = depth }};
    arrput(*stmt, node);
    node = (Stmt){ .position = { .line = line, .column = column }};
    arrput(*stmt, node);
    node = (Stmt){ .length = length };
    arrput(*stmt, node);
}

Stmt* parser_parse_stmts(Parser* parser, int depth)
{
    Stmt* curr_stmt = NULL;
    Stmt* recv_stmt = NULL;

    while (true)
    {
        recv_stmt = parser_parse_stmt(parser, depth + 1);
        if (recv_stmt == NULL)
            break;

        append_list_to_list(curr_stmt, recv_stmt);
        arrfree_and_set_null(recv_stmt);
    }

    return curr_stmt;
}

Stmt* parser_parse_stmt(Parser* parser, int depth)
{
    Token tok;
    Stmt* curr_stmt = NULL;
    Stmt* recv_stmt = NULL;

    Stmt identifier_node;
    Stmt type_node;
    Stmt expr_node;
    Stmt err_node;

    ExprOp* type_ptr = NULL;
    ExprOp* expr_ptr = NULL;
    char*   identifier_ptr = NULL;

    parser_skip(parser, is_newline);

    tok = parser_peek(parser);
    switch (tok.type)
    {
        case TOKEN_LET:

            parser_next(parser);
            identifier_ptr = parser_parse_identifier(parser);

            if (parser_peek(parser).type == TOKEN_COLON)
            {
                parser_next(parser);
                type_ptr = parser_parse_type(parser);

                if (parser_peek(parser).type == TOKEN_EQUAL)
                {
                    parser_next(parser);
                    expr_ptr = parser_parse_expr(parser);

                    stmt_append_header(&curr_stmt, STMT_LET_TYPE_AND_EXPR, depth, tok.line, tok.column, 0);
                    identifier_node = (Stmt){ .literal = identifier_ptr };
                    type_node       = (Stmt){ .expr   = type_ptr };
                    expr_node       = (Stmt){ .expr   = expr_ptr };
                    arrput(curr_stmt,  identifier_node);
                    arrput(curr_stmt, type_node);
                    arrput(curr_stmt, expr_node);
                }
                else
                {
                    stmt_append_header(&curr_stmt, STMT_LET_TYPE, depth, tok.line, tok.column, 0);
                    identifier_node = (Stmt){ .literal = identifier_ptr };
                    type_node       = (Stmt){ .expr = type_ptr };
                    arrput(curr_stmt,  identifier_node);
                    arrput(curr_stmt, type_node);
                }
            }
            else
            {
                if (parser_peek(parser).type == TOKEN_EQUAL)
                {
                    parser_next(parser);
                    expr_ptr = parser_parse_expr(parser);

                    stmt_append_header(&curr_stmt, STMT_LET_EXPR, depth, tok.line, tok.column, 0);
                    identifier_node = (Stmt){ .literal = identifier_ptr };
                    expr_node       = (Stmt){ .expr    = expr_ptr };
                    arrput(curr_stmt,  identifier_node);
                    arrput(curr_stmt, expr_node);
                }
                else
                {
                    stmt_append_header(&curr_stmt, STMT_LET_BARE, depth, tok.line, tok.column, 0);
                    identifier_node = (Stmt){ .literal  = identifier_ptr };
                    arrput(curr_stmt,  identifier_node);
                }
            }
            return curr_stmt;

        case TOKEN_IF:
            parser_next(parser);
            expr_ptr = parser_parse_expr(parser);
            
            stmt_append_header(&curr_stmt, STMT_IF, depth, tok.line, tok.column, 0);
            expr_node = (Stmt){ .expr = expr_ptr };
            arrput(curr_stmt, expr_node);

            tok = parser_peek(parser);
            if (is_newline(tok.type))
            {
                parser_skip(parser, is_newline);
                recv_stmt = parser_parse_stmt(parser, depth + 1);
                append_list_to_list(curr_stmt, recv_stmt);
                arrfree_and_set_null(recv_stmt);
            }
            else if (tok.type == TOKEN_DO)
            {
                recv_stmt = parser_parse_block(parser, depth + 1);
                if (recv_stmt == NULL)
                {
                    err_node = (Stmt){ .header = { .type = STMT_EMPTY, .depth = depth + 1}};
                    arrput(curr_stmt, err_node);
                }
                else
                {
                    append_list_to_list(curr_stmt, recv_stmt);
                    arrfree_and_set_null(recv_stmt);
                }
            }
            else
            {
                stmt_append_header(&curr_stmt, STMT_ERR, depth, tok.line, tok.column, 0);
            }

            return curr_stmt;
        case TOKEN_WHILE:
            parser_next(parser);
            expr_ptr = parser_parse_expr(parser);
            
            stmt_append_header(&curr_stmt, STMT_WHILE, depth, tok.line, tok.column, 0);
            expr_node = (Stmt){ .expr = expr_ptr };
            arrput(curr_stmt, expr_node);

            tok = parser_peek(parser);
            if (is_newline(tok.type))
            {
                parser_skip(parser, is_newline);
                recv_stmt = parser_parse_stmt(parser, depth + 1);
                append_list_to_list(curr_stmt, recv_stmt);
                arrfree_and_set_null(recv_stmt);
            }
            else if (tok.type == TOKEN_DO)
            {
                recv_stmt = parser_parse_block(parser, depth + 1);
                if (recv_stmt == NULL)
                {
                    err_node = (Stmt){ .header = { .type = STMT_EMPTY, .depth = depth + 1}};
                    arrput(curr_stmt, err_node);
                }
                else
                {
                    append_list_to_list(curr_stmt, recv_stmt);
                    arrfree_and_set_null(recv_stmt);
                }
            }
            else
            {
                stmt_append_header(&curr_stmt, STMT_ERR, depth, tok.line, tok.column, 0);
            }

            return curr_stmt;

        case TOKEN_DO:
            //perror("Parser line %d: Not implemented yet.\n");
            //exit(1);
            recv_stmt = parser_parse_block(parser, depth + 1);
            return recv_stmt;

        case TOKEN_ERROR:
            return NULL;
        default:
            perror("Parser line %d: Failed to parse statement\n");
            exit(1);
    }
}

char* parser_parse_identifier(Parser* parser)
{
    Token token = parser_next(parser);
    if (token.type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Parser line %d: The next token is not an identifier.\n", __LINE__);
        exit(1);
    }

    char* identifier = calloc((size_t)token.length + 1, sizeof(char)); // This sets everything to '\0'
    if (identifier == NULL)
    {
        fprintf(stderr, "Failed to allocate memory in %s:%d.\n", __FILE__, __LINE__);
        exit(1);
    }
    arrput(parser->identifiers, identifier);
    return identifier;
}

Stmt* parser_parse_block(Parser* parser, int depth)
{
    fprintf(stderr, "Parser line %d: Not implemented yet.\n", __LINE__);
    exit(1);
}

ExprOp* parser_parse_expr(Parser* parser)
{
    ExprOp* expr  = parser_parse_expr_inner(parser, 0);
    return expr;
}

// TODO: Rework this to add literals to parser storage.
// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
#define MAKE_OP_FROM_TOKEN(token, oper_type, argsn, expr_type) (ExprOp){\
        .op_type = (oper_type),\
        .type = (expr_type),\
        .args = (argsn),\
        .literal = (token).start,\
        .line = (token).line,\
        .column = (token).column,\
        .length = (token).length\
    } 
ExprOp* parser_parse_expr_inner(Parser* parser, int min_binding_power)
{
    ExprOp* expr = NULL;
    Token token;

    ExprOp  lhs;
    ExprOp  op ;
    ExprOp* rhs = NULL;
    int left_binding_power, right_binding_power;

    // Parse first literal
    token = parser_next(parser);
    LhsOpType lot;
    switch (token.type)
    {
        // Atoms
        case TOKEN_NIL       : lhs = MAKE_OP_FROM_TOKEN(token, OP_NIL       , 0, TYPE_NIL    ); lot = LHS_OP_TYPE_ATOM; break;
        case TOKEN_TRUE      : lhs = MAKE_OP_FROM_TOKEN(token, OP_TRUE      , 0, TYPE_BOOL   ); lot = LHS_OP_TYPE_ATOM; break;
        case TOKEN_FALSE     : lhs = MAKE_OP_FROM_TOKEN(token, OP_FALSE     , 0, TYPE_BOOL   ); lot = LHS_OP_TYPE_ATOM; break;
        case TOKEN_NUMBER    : lhs = MAKE_OP_FROM_TOKEN(token, OP_NUMBER    , 0, TYPE_FLOAT  ); lot = LHS_OP_TYPE_ATOM  ; break;
        case TOKEN_INTEGER   : lhs = MAKE_OP_FROM_TOKEN(token, OP_INTEGER   , 0, TYPE_INT    ); lot = LHS_OP_TYPE_ATOM  ; break;
        case TOKEN_STRING    : lhs = MAKE_OP_FROM_TOKEN(token, OP_STRING    , 0, TYPE_UNKNOWN); lot = LHS_OP_TYPE_ATOM  ; break;
        case TOKEN_IDENTIFIER: lhs = MAKE_OP_FROM_TOKEN(token, OP_IDENTIFIER, 0, TYPE_UNKNOWN); lot = LHS_OP_TYPE_ATOM  ; break;
        case TOKEN_PRINT     : lhs = MAKE_OP_FROM_TOKEN(token, OP_PRINT     , 0, TYPE_UNKNOWN); lot = LHS_OP_TYPE_ATOM  ; break;
        // Ops
        case TOKEN_MINUS       : op = MAKE_OP_FROM_TOKEN(token, OP_NEG    , 1, TYPE_UNKNOWN); lot = LHS_OP_TYPE_PREFIX; break;
        case TOKEN_BANG        : op = MAKE_OP_FROM_TOKEN(token, OP_NOT    , 1, TYPE_UNKNOWN); lot = LHS_OP_TYPE_PREFIX; break;
        case TOKEN_PLUS_PLUS   : op = MAKE_OP_FROM_TOKEN(token, OP_PRE_INC, 1, TYPE_UNKNOWN); lot = LHS_OP_TYPE_PREFIX; break;
        case TOKEN_MINUS_MINUS : op = MAKE_OP_FROM_TOKEN(token, OP_PRE_DEC, 1, TYPE_UNKNOWN); lot = LHS_OP_TYPE_PREFIX; break;
        // Special
        case TOKEN_LEFT_PAREN: lot = LHS_OP_TYPE_PARENS; break;
        default:
            fprintf(stderr, "Parser line %d: Could not find atomic expression.\n", __LINE__);
            exit(1);
    }
    switch (lot)
    {
        case LHS_OP_TYPE_ATOM:
            arrput(expr, lhs);
            break;

        case LHS_OP_TYPE_PREFIX:
            prefix_binding_power(op.op_type, &right_binding_power);
            rhs = parser_parse_expr_inner(parser, right_binding_power);

            arrput(expr, op);
            append_rhs_to_expr(&expr, &rhs);
            arrfree_and_set_null(rhs);
            break;

        case LHS_OP_TYPE_PARENS:
            token = parser_peek(parser);
            if (token.type == TOKEN_RIGHT_PAREN)
            {
                fprintf(stderr, "Parser line %d: Nothing found in parentheses.\n", __LINE__);
                exit(1);
            }
            rhs = parser_parse_expr_inner(parser, 0);

            append_rhs_to_expr(&expr, &rhs);
            arrfree_and_set_null(rhs);
            parser_next(parser);
            break;
        default:
            fprintf(stderr, "Parser line %d: Unknown LhsOpType value.\n", __LINE__);
            exit(1);
    }
    
    while (true)
    {
        bool should_break_loop = false;
        // Parse binary operator
        token = parser_peek(parser);
        switch (token.type)
        {
            case TOKEN_PLUS         : op = MAKE_OP_FROM_TOKEN(token, OP_ADD          , 2, TYPE_UNKNOWN); break;
            case TOKEN_MINUS        : op = MAKE_OP_FROM_TOKEN(token, OP_SUB          , 2, TYPE_UNKNOWN); break;
            case TOKEN_STAR         : op = MAKE_OP_FROM_TOKEN(token, OP_MUL          , 2, TYPE_UNKNOWN); break;
            case TOKEN_SLASH        : op = MAKE_OP_FROM_TOKEN(token, OP_DIV          , 2, TYPE_UNKNOWN); break;
            case TOKEN_PERCENT      : op = MAKE_OP_FROM_TOKEN(token, OP_MOD          , 2, TYPE_UNKNOWN); break;
            case TOKEN_AND          : op = MAKE_OP_FROM_TOKEN(token, OP_AND          , 2, TYPE_UNKNOWN); break;
            case TOKEN_OR           : op = MAKE_OP_FROM_TOKEN(token, OP_OR           , 2, TYPE_UNKNOWN); break;
            case TOKEN_EQUAL_EQUAL  : op = MAKE_OP_FROM_TOKEN(token, OP_EQUAL        , 2, TYPE_UNKNOWN); break;
            case TOKEN_BANG_EQUAL   : op = MAKE_OP_FROM_TOKEN(token, OP_NOT_EQUAL    , 2, TYPE_UNKNOWN); break;
            case TOKEN_GREATER      : op = MAKE_OP_FROM_TOKEN(token, OP_GREATER      , 2, TYPE_UNKNOWN); break;
            case TOKEN_LESS         : op = MAKE_OP_FROM_TOKEN(token, OP_LESS         , 2, TYPE_UNKNOWN); break;
            case TOKEN_GREATER_EQUAL: op = MAKE_OP_FROM_TOKEN(token, OP_GREATER_EQUAL, 2, TYPE_UNKNOWN); break;
            case TOKEN_LESS_EQUAL   : op = MAKE_OP_FROM_TOKEN(token, OP_LESS_EQUAL   , 2, TYPE_UNKNOWN); break;
            case TOKEN_DOT          : op = MAKE_OP_FROM_TOKEN(token, OP_ACCESS       , 2, TYPE_UNKNOWN); break;
            case TOKEN_COLON        : op = MAKE_OP_FROM_TOKEN(token, OP_CHAIN        , 2, TYPE_UNKNOWN); break;
            case TOKEN_EQUAL        : op = MAKE_OP_FROM_TOKEN(token, OP_ASSIGN       , 2, TYPE_UNKNOWN); break;
            case TOKEN_PLUS_EQUAL   : op = MAKE_OP_FROM_TOKEN(token, OP_ASSIGN_ADD   , 2, TYPE_UNKNOWN); break;
            case TOKEN_MINUS_EQUAL  : op = MAKE_OP_FROM_TOKEN(token, OP_ASSIGN_SUB   , 2, TYPE_UNKNOWN); break;
            case TOKEN_STAR_EQUAL   : op = MAKE_OP_FROM_TOKEN(token, OP_ASSIGN_MUL   , 2, TYPE_UNKNOWN); break;
            case TOKEN_PERCENT_EQUAL: op = MAKE_OP_FROM_TOKEN(token, OP_ASSIGN_MOD   , 2, TYPE_UNKNOWN); break;
            case TOKEN_PLUS_PLUS    : op = MAKE_OP_FROM_TOKEN(token, OP_POST_INC     , 1, TYPE_UNKNOWN); break;
            case TOKEN_MINUS_MINUS  : op = MAKE_OP_FROM_TOKEN(token, OP_POST_DEC     , 1, TYPE_UNKNOWN); break;
            case TOKEN_LEFT_SQUARE  : op = MAKE_OP_FROM_TOKEN(token, OP_INDEX        , 2, TYPE_UNKNOWN); break;
            case TOKEN_LEFT_PAREN   : op = MAKE_OP_FROM_TOKEN(token, OP_CALL         , 1, TYPE_UNKNOWN); break;
            default:
                should_break_loop = true;
        }
        if (should_break_loop) break;

        // Postfix
        if (postfix_binding_power(op.op_type, &left_binding_power))
        {
            if (left_binding_power < min_binding_power)
                break;
            parser_next(parser); // We probably want to do this now.

            if (op.op_type == OP_INDEX)
            {
                token = parser_peek(parser);
                if (token.type == TOKEN_RIGHT_SQUARE)
                {
                    fprintf(stderr, "Parser line %d: Nothing found in square braces.\n", __LINE__);
                    exit(1);
                }
                rhs = parser_parse_expr_inner(parser, 0);
                token = parser_next(parser); // Skip second bracket
                if (token.type == TOKEN_RIGHT_SQUARE)
                {
                    fprintf(stderr, "Parser line %d: Expected right square brace.\n", __LINE__);
                    exit(1);
                }

                arrins(expr, 0, op);
                append_rhs_to_expr(&expr, &rhs);
                arrfree_and_set_null(rhs);
            }
            else if (op.op_type == OP_CALL)
            {
                token = parser_peek(parser);
                if (token.type != TOKEN_RIGHT_PAREN)
                {
                    do
                    {
                        rhs = parser_parse_expr_inner(parser, 0);

                        append_rhs_to_expr(&expr, &rhs);
                        arrfree_and_set_null(rhs);

                        op.args++;

                        token = parser_next(parser); // Skip second bracket

                        if (token.type == TOKEN_RIGHT_PAREN)
                            break;
                        else if (token.type == TOKEN_COMMA)
                            continue;
                        else
                        {
                            fprintf(stderr, "Parser line %d: Function arguments have to be seperated by commas.\n", __LINE__);
                            exit(1);
                        }
                    }
                    while(true);
                }
                arrins(expr, 0, op);
            }
            else
            {
                arrins(expr, 0, op);
                //fprintf(stderr, "Parser line %d: Postfix parsing not implemented.\n", __LINE__);
                //exit(1);
            }

            continue;
        }

        // Infix
        if (infix_binding_power(op.op_type, &left_binding_power, &right_binding_power))
        {
            if (left_binding_power < min_binding_power)
                break;

            parser_next(parser);
            rhs = parser_parse_expr_inner(parser, right_binding_power);
            
            arrins(expr, 0, op);
            append_rhs_to_expr(&expr, &rhs);
            arrfree_and_set_null(rhs);
            continue;
        }

        break;
    }

    // print_expr_op(expr);
    return expr;

}

ExprOp* parser_parse_type(Parser* parser)
{
    ExprOp* type = parser_parse_type_inner(parser, 0);
    return type;
}

// TODO: Rework this to add literals to parser storage.
ExprOp* parser_parse_type_inner(Parser* parser, int min_binding_power)
{
    ExprOp* expr = NULL;
    Token token;

    ExprOp  lhs;

    // Parse first literal
    token = parser_next(parser);
    switch (token.type)
    {
        // Atoms
        case TOKEN_IDENTIFIER: lhs = MAKE_OP_FROM_TOKEN(token, OP_IDENTIFIER, 0, TYPE_UNKNOWN); break;
        default:
            fprintf(stderr, "Parser line %d: Could not find atomic expression.\n", __LINE__);
            exit(1);
    }
    arrput(expr, lhs);
    
    // print_expr_op(expr);
    return expr;

}
#undef MAKE_OP_FROM_TOKEN

void prefix_binding_power(ExprOpType op_type, int* right)
{
    switch (op_type)
    {
        case OP_NEG:
        case OP_NOT:
        case OP_PRE_INC:
        case OP_PRE_DEC:
            *right = 11;
            break;

        default:
            fprintf(stderr, "Parser line %d: Operator is not prefix.\n", __LINE__);
            exit(1);
    }
}

bool postfix_binding_power(ExprOpType op_type, int* left)
{
    switch (op_type)
    {
        case OP_INDEX:
        case OP_CALL:
            *left = 15;
            break;

        case OP_POST_INC:
        case OP_POST_DEC:
            *left = 13;
            break;

        default:
            return false;
    }

    return true;
}

bool infix_binding_power(
    ExprOpType op_type,
    int* left,
    int* right)
{
    switch (op_type)
    {
        case OP_ASSIGN:
        case OP_ASSIGN_ADD:
        case OP_ASSIGN_SUB:
        case OP_ASSIGN_MUL:
        case OP_ASSIGN_MOD:
            *left  = 2;
            *right = 1;
            break;

        case OP_OR:
            *left  = 3;
            *right = 4;
            break;

        case OP_AND:
            *left  = 5;
            *right = 6;
            break;

        case OP_EQUAL:
        case OP_NOT_EQUAL:
            *left  = 7;
            *right = 8;
            break;

        case OP_GREATER:
        case OP_LESS:
        case OP_GREATER_EQUAL:
        case OP_LESS_EQUAL:
            *left  = 9;
            *right = 10;
            break;

        case OP_ADD:
        case OP_SUB:
            *left  = 11;
            *right = 12;
            break;

        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
            *left  = 13;
            *right = 14;
            break;

        case OP_ACCESS:
        case OP_CHAIN:
            *left  = 17;
            *right = 16;
            break;

        default:
            return false;
    }

    return true;
}

void append_rhs_to_expr(ExprOp** expr, ExprOp** rhs)
{
    int rhs_len = arrlen(*rhs);
    for (int i = 0; i < rhs_len; ++i)
    {
        ExprOp r = *rhs[i];
        arrput(*expr, r);
    }
}

// TODO: Implement later
// ExprOp* parser_parse_expr(Parser* parser)
// {
//     return ARENA_NULL;
// }

const char* show_op_type(ExprOpType op)
{
    switch (op)
    {
        case OP_STRING: return "OP_STRING";
        case OP_IDENTIFIER: return "OP_IDENTIFIER";
        case OP_INTEGER: return "OP_INTEGER";
        case OP_NUMBER: return "OP_NUMBER";
        case OP_TRUE: return "OP_TRUE";
        case OP_FALSE: return "OP_FALSE";
        case OP_NIL: return "OP_NIL";
        case OP_PRINT: return "OP_PRINT";
        case OP_NEG: return "OP_NEG";
        case OP_NOT: return "OP_NOT";
        case OP_PRE_INC: return "OP_PRE_INC";
        case OP_PRE_DEC: return "OP_PRE_DEC";
        case OP_INDEX: return "OP_INDEX";
        case OP_POST_INC: return "OP_POST_INC";
        case OP_POST_DEC: return "OP_POST_DEC";
        case OP_ADD: return "OP_ADD";
        case OP_SUB: return "OP_SUB";
        case OP_MUL: return "OP_MUL";
        case OP_DIV: return "OP_DIV";
        case OP_MOD: return "OP_MOD";
        case OP_AND: return "OP_AND";
        case OP_OR: return "OP_OR"; 
        case OP_GREATER: return "OP_GREATER";
        case OP_LESS: return "OP_LESS";
        case OP_GREATER_EQUAL: return "OP_GREATER_EQUAL";
        case OP_LESS_EQUAL: return "OP_LESS_EQUAL";
        case OP_EQUAL: return "OP_EQUAL";
        case OP_NOT_EQUAL: return "OP_NOT_EQUAL";
        case OP_ACCESS: return "OP_ACCESS";
        case OP_CHAIN: return "OP_CHAIN";
        case OP_ASSIGN: return "OP_ASSIGN";
        case OP_ASSIGN_ADD: return "OP_ASSIGN_ADD";
        case OP_ASSIGN_SUB: return "OP_ASSIGN_SUB";
        case OP_ASSIGN_MUL: return "OP_ASSIGN_MUL";
        case OP_ASSIGN_MOD: return "OP_ASSIGN_MOD";
        case OP_CALL: return "OP_CALL";
        default:
            return "UNKNOWN";
    }
}

void print_expr_op(ExprOp* op)
{
    int len = arrlen(op);
    printf("Expr (len = %d):\n", len);
    for (int i = 0; i < len; ++i)
    {
        ExprOp e = op[i];
        const char* type = show_op_type(e.op_type); 
        printf(
            "[%d:%d:%d]: '%.*s'\n",
            e.line   ,
            e.column ,
            e.length ,
            e.length ,
            e.literal
        );
    }
}

const char* stmt_type_name(StmtType type)
{
    switch (type)
    {
        case STMT_ERR:       return "STMT_ERR";
        case STMT_LET_BARE:  return "STMT_LET_BARE";
        case STMT_LET_TYPE:  return "STMT_LET_TYPE";
        case STMT_LET_EXPR:  return "STMT_LET_EXPR";
        case STMT_LET_TYPE_AND_EXPR: return "STMT_LET_TYPE_AND_EXPR";
        case STMT_EXPR:              return "STMT_EXPR";
        case STMT_IF:                return "STMT_IF";
        case STMT_ELIF:              return "STMT_ELIF";
        case STMT_ELSE:              return "STMT_ELSE";
        case STMT_WHILE:             return "STMT_WHILE";
        case STMT_BREAK:             return "STMT_BREAK";
        case STMT_CONTINUE:          return "STMT_CONTINUE";
        case STMT_FN:                return "STMT_FN";
        case STMT_RETURN:            return "STMT_RETURN";
        case STMT_EMPTY:             return "STMT_EMPTY";
        default:                     return "UNKNOWN";
    }
}

void print_stmt(Stmt* stmt)
{
    if (stmt == NULL)
    {
        printf("<null stmt>\n");
        return;
    }

    StmtType type = stmt[0].header.type;
    int depth     = stmt[0].header.depth;

    int line      = stmt[1].position.line;
    int column    = stmt[1].position.column;

    printf("%s (depth=%d, line=%d, column=%d)\n",
           stmt_type_name(type),
           depth,
           line,
           column);

    switch (type)
    {
        case STMT_LET_BARE:
        {
            printf("  identifier = %s\n",
                   stmt[3].literal);
            break;
        }

        case STMT_LET_TYPE:
        {
            printf("  identifier = %s\n",
                   stmt[3].literal);

            printf("  type expr:\n");
            print_expr_op(stmt[4].expr);
            break;
        }

        case STMT_LET_EXPR:
        {
            printf("  identifier = %s\n",
                   stmt[3].literal);

            printf("  value expr:\n");
            print_expr_op(stmt[4].expr);
            break;
        }

        case STMT_LET_TYPE_AND_EXPR:
        {
            printf("  identifier = %s\n",
                   stmt[3].literal);

            printf("  type expr:\n");
            print_expr_op(stmt[4].expr);

            printf("  value expr:\n");
            print_expr_op(stmt[5].expr);
            break;
        }

        case STMT_IF:
        {
            printf("  if statement\n");
            break;
        }

        case STMT_WHILE:
        {
            printf("  while statement\n");
            break;
        }

        case STMT_EMPTY:
        {
            printf("  empty\n");
            break;
        }

        case STMT_ERR:
        {
            printf("  error node\n");
            break;
        }

        default:
        {
            printf("  printer not implemented for this stmt type\n");
            break;
        }
    }
}
