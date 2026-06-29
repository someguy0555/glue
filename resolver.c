#include "resolver.h"

Stmt* resolver_resolve_stmt(Resolver* resolver)
{
    Stmt* curr_stmt  = NULL;

    char * identifier = NULL;
    Type * type       = NULL;
    Expr * expr       = NULL;
    Stmt * stmt       = NULL;
    int    size       = 0   ;
    Stmt** body       = NULL;
    StmtFn fn;

    curr_stmt = resolver->stmts;
    if (curr_stmt == NULL)
    {
        fprintf(stderr, "[%s:%d] Variable resolution: Found NULL statement.\n", __FILE__, __LINE__);
        exit(1);
    }

    switch (curr_stmt->kind)
    {
        case STMT_ERR     :
            fprintf(stderr, "[%s:%d] Variable resolution: Found statement of kind STMT_ERR.\n", __FILE__, __LINE__);
            exit(1);
            break;

        case STMT_BLOCK   :
            size = curr_stmt->stmt.block.size;
            body = curr_stmt->stmt.block.body;

            for (int i = 0; i < size; ++i)
            {
                Stmt* stmt = body[0];

                resolver->stmts = stmt     ;
                resolver_resolve_stmt(resolver);
                resolver->stmts = curr_stmt;
            }
            break;

        case STMT_LET     :
            identifier = curr_stmt->stmt.let.identifier;
            type       = curr_stmt->stmt.let.type      ;
            expr       = curr_stmt->stmt.let.expr      ;

            resolver_declare_let(resolver, identifier, type, expr);

            resolver_resolve_expr(resolver, expr);
            if (expr == NULL)
            {
                return NULL;
            }
            break;

        case STMT_EXPR    :
            expr = curr_stmt->stmt.expr;

            resolver_resolve_expr(resolver, expr);
            if (expr == NULL)
            {
                return NULL;
            }
            break;

        case STMT_IF      :
            expr = curr_stmt->stmt.iff.condition;
            stmt = curr_stmt->stmt.iff.body     ;

            resolver_resolve_expr(resolver, expr);
            if (expr == NULL)
            {
                return NULL;
            }

            resolver->stmts = stmt     ;
            resolver_resolve_stmt(resolver);
            resolver->stmts = curr_stmt;
            break;

        case STMT_WHILE   :
            expr = curr_stmt->stmt.whilee.condition;
            stmt = curr_stmt->stmt.whilee.body     ;

            resolver_resolve_expr(resolver, expr);
            if (expr == NULL)
            {
                return NULL;
            }

            resolver->stmts = stmt     ;
            resolver->loop_depth++;
            resolver_resolve_stmt(resolver);
            resolver->loop_depth--;
            resolver->stmts = curr_stmt;
            break;

        case STMT_BREAK   :
            if (resolver->loop_depth <= 0)
            {
                resolver_throw_compiler_error(resolver, (CompileError)
                {
                    .kind   = ERROR_ERROR      ,
                    .line   = curr_stmt->line  ,
                    .column = curr_stmt->column,
                    .length = curr_stmt->line  ,
                    .msg    = "Statement resolution: Breaking while not in loop.",
                });
                return NULL;
            }
            break;

        case STMT_CONTINUE:
            if (resolver->loop_depth <= 0)
            {
                resolver_throw_compiler_error(resolver, (CompileError)
                {
                    .kind   = ERROR_ERROR      ,
                    .line   = curr_stmt->line  ,
                    .column = curr_stmt->column,
                    .length = curr_stmt->line  ,
                    .msg    = "Statement resolution: Continuing while not in loop.",
                });
                return NULL;
            }
            break;

        case STMT_FN      :
            fn = curr_stmt->stmt.fn;
            stmt = fn.body;

            resolver_declare_fn(resolver, fn);

            resolver->stmts = stmt     ;
            resolver_resolve_stmt(resolver);
            resolver->stmts = curr_stmt;
            break;

        case STMT_RETURN  :
            // TODO: Somehow bind this to it's respective function declaration.
            if (!resolver->inside_function)
            {
                resolver_throw_compiler_error(resolver, (CompileError)
                {
                    .kind   = ERROR_ERROR      ,
                    .line   = curr_stmt->line  ,
                    .column = curr_stmt->column,
                    .length = curr_stmt->line  ,
                    .msg    = "Statement resolution: Cannot return while not in function.",
                });
                return NULL;
            }
            assert(false);
            break;

        default:
            fprintf(stderr, "[%s:%d] Variable resolution: Found statement of unknown kind.\n", __FILE__, __LINE__);
            exit(1);
    }

    return curr_stmt;
}

// Set variable to the biggest instance of identifier.
void resolver_resolve_expr(Resolver* resolver, Expr* expr)
{
    fprintf(stderr, "[%s:%d] Not implemented yet.\n", __FILE__, __LINE__);
    exit(1);
}

Decl* resolver_declare_let(Resolver* resolver, char* identifier, Type* type, Expr* expr)
{
    Decl  decl;
    Decl* decl_ptr = NULL;

    Variable  var;
    Variable* var_ptr = NULL;

    char* existing_identifier = NULL;

    existing_identifier = resolver_get_identifier(resolver, identifier);

    var = (Variable)
    {
        .identifier = existing_identifier,
        .type       = type               ,
    };

    var_ptr = (Variable*) arena_push(&resolver->arena, &var, sizeof(Variable));

    decl = (Decl)
    {
        .kind = DECL_LET,
        .decl.let = (DeclLet)
        {
            .variable = var_ptr,
        }
    };

    decl_ptr = (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
    arrput(resolver->declarations, decl_ptr);

    return decl_ptr;
    // fprintf(stderr, "[%s:%d] Not implemented yet.\n", __FILE__, __LINE__);
    // exit(1);
}

// TODO: Make it so that we cannot have function arguments that have the same identifier as the function name.
Decl* resolver_declare_fn(Resolver* resolver, StmtFn fn)
{
    Decl  decl;
    Decl* decl_ptr = NULL;

    Variable  var;
    Variable* var_ptr = NULL;

    Type* fn_type = NULL;

    int argc = 0;
    Variable** argv = NULL;

    char* existing_identifier = NULL;

    existing_identifier = resolver_get_identifier(resolver, fn.identifier);

    Type tp = construct_fn_type(fn);
    fn_type = (Type*) arena_push(&resolver->arena, &tp, sizeof(Type));
    var = (Variable)
    {
        .identifier = existing_identifier,
        .type       = fn_type            ,
    };

    var_ptr = (Variable*) arena_push(&resolver->arena, &var, sizeof(Variable));

    for (int i = 0; i < fn.argc; ++i)
    {
        Decl* d = NULL;
        StmtFnArg arg = *(fn.argv[i]);
        d = resolver_declare_let(resolver, arg.identifier, arg.type, NULL);
        assert(d->kind == DECL_LET);
        arrput(argv, d->decl.let.variable);
    }

    argc = arrlen(argv);
    Variable** tmp_ptr = argv;
    argv = (Variable**) arena_push(&resolver->arena, argv, argc * sizeof(Variable*));
    arrfree(tmp_ptr);

    decl = (Decl)
    {
        .kind = DECL_FN,
        .decl.fn = (DeclFn)
        {
            .variable = var_ptr,
            .argc     = argc   ,
            .argv     = argv   ,
        }
    };

    // struct StmtFnArg
    // {
        // char*   identifier;
        // Type*   type      ;
    // };

    // struct StmtFn
    // {
        // char     *  identifier ;
        // int         argc       ;
        // StmtFnArg** argv       ;
        // Type     *  return_type;
        // Stmt     *  body       ;
    // };

    decl_ptr = (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
    arrput(resolver->declarations, decl_ptr);

    return decl_ptr;
    // fprintf(stderr, "[%s:%d] Not implemented yet.\n", __FILE__, __LINE__);
    // exit(1);
}

char* resolver_get_identifier(Resolver* resolver, char* identifier)
{
    int id_len = strlen(identifier);
    int len = arrlen(resolver->identifiers);
    char* new_id = NULL;

    for (int i = 0; i < len; ++i)
    {
        char* id = resolver->identifiers[i];
        if (memcmp(id, identifier, id_len) == 0)
        {
            return id;
        }
    }

    new_id = (char*) arena_push(&resolver->arena, identifier, id_len * sizeof(char));
    arrput(resolver->identifiers, new_id);

    return new_id;
}

void resolver_throw_compiler_error(Resolver* resolver, CompileError err)
{
    CompileError* err_ptr = NULL;
    err_ptr = (CompileError*) arena_push(&resolver->arena, &err, sizeof(CompileError));
    arrput(resolver->errs, err_ptr);
}

Type construct_fn_type(StmtFn fn)
{
    fprintf(stderr, "[%s:%d] Not implemented yet.\n", __FILE__, __LINE__);
    exit(1);
}
