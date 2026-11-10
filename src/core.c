#include <core.h>

DefStringMapImpl(ConfigMap, ConfigVal);

#ifdef DEBUG
#define INTERNAL_ONLY
#else
#define INTERNAL_ONLY static
#endif

#ifndef DEBUG
typedef enum TokenType {
    TOKEN_INVALID = 0, //uninitialized tokens are invalid
    TOKEN_ID,
    TOKEN_ANY,
    TOKEN_NUM,
    TOKEN_TAG,
    TOKEN_AND,
    TOKEN_NOT,
    TOKEN_OR,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_LE,
    TOKEN_GE,
    TOKEN_OPAREN,
    TOKEN_CPAREN,
    TOKEN_END,
} TokenType;

const SString token_names[] = {
    [TOKEN_ID]      = sstring("ID"),
    [TOKEN_NUM]     = sstring("NUM"),
    [TOKEN_TAG]     = sstring("TAG"),
    [TOKEN_AND]     = sstring("AND"),
    [TOKEN_NOT]     = sstring("NOT"),
    [TOKEN_ANY]     = sstring("ANY"),
    [TOKEN_OR]      = sstring("OR"),

    [TOKEN_LT]      = sstring("LT"),
    [TOKEN_GT]      = sstring("GT"),
    [TOKEN_EQ]      = sstring("EQ"),
    [TOKEN_NEQ]     = sstring("NEQ"),
    [TOKEN_LE]      = sstring("LE"),
    [TOKEN_GE]      = sstring("GE"),

    [TOKEN_OPAREN]  = sstring("OPAREN"),
    [TOKEN_CPAREN]  = sstring("CPAREN"),
    [TOKEN_END]     = sstring("END"),
    [TOKEN_INVALID] = sstring("INVALID"),
};

typedef struct Token {
    TokenType t;
    SString token;
} Token;

typedef struct ExprTokenizer {
    char* data;
    u32 curr;
    u32 max;
    Token token;
} ExprTokenizer;


#include "setjmp.h"

typedef enum ErrorCode {
    ERROR_NO_ERROR = 0,
    ERROR_INVALID_UNARY,
    ERROR_INVALID_ID,
    ERROR_INVALID_TOKEN,
    ERROR_UNEXPECTED_TOKEN,
} ErrorCode;

typedef struct ParsingData {
    ExprTokenizer tok;
    dynArray(char) query;

    struct {
        ErrorCode code;
        u32 index;
        jmp_buf handler;
    } err;
    
} ParsingData;

static const SString errormsgs[] = {
    [ERROR_NO_ERROR]            = sstring("No Error: Should never happen"),
    [ERROR_INVALID_UNARY]       = sstring("Invalid Unary: Expected TAG, NOT, ANY, or '(' Got %s"),
    [ERROR_INVALID_ID]          = sstring("Invalid %s: Got '%s'"),
    [ERROR_INVALID_TOKEN]       = sstring("Invalid Token"),
    [ERROR_UNEXPECTED_TOKEN]    = sstring("Unexpected Token: Got %s"),
};

#endif

//return TRUE if sucess, FALSE if failed
bool8 PopChar(ExprTokenizer* tok) {
    tok->curr++;
    if (tok->curr > tok->max) {
        return FALSE;
    }
    return TRUE;
}

//BNF
//
// EXPR := OEXPR
//      |  OPEXR AND EXPR
//
// OEXPR := CEXPR
//       |  CEXPR OR OEXPR
//       |  UNARY OR OEXPR
//
// CEXPR := ID
//       |  ID CMP CEXPR
//
// CMP := LT
//     |  GT
//     |  EQ
//     |  NEQ
//     |  LE
//     |  GE
//
// UNARY := NOT UNARY
//        | TAG
//        | ANY
//        | '(' EXPR ')'
//
// TAG := ':' String
//
// ID := 'ANY'
//     | 'PRIORITY'
//     | 'COMPLETION'
//

INTERNAL_ONLY void _NextToken(ExprTokenizer* t) {
    //Eat Whitespace
    while (isSpace(t->data[t->curr]) && PopChar(t));

    if (t->curr >= t->max) {
        PopChar(t);
        t->token = (Token){TOKEN_END};
        return;
    }

    if (t->max - t->curr >= 2 && t->data[t->curr] == '!' && t->data[t->curr + 1] == '=') {
        PopChar(t);
        PopChar(t);
        t->token = (Token){ .t = TOKEN_NEQ };
        return;
    }

    if (t->max - t->curr >= 2 && t->data[t->curr] == '<' && t->data[t->curr + 1] == '=') {
        PopChar(t);
        PopChar(t);
        t->token = (Token){ .t = TOKEN_LE };
        return;
    }

    if (t->max - t->curr >= 2 && t->data[t->curr] == '>' && t->data[t->curr + 1] == '=') {
        PopChar(t);
        PopChar(t);
        t->token = (Token){ .t = TOKEN_GE };
        return;
    }

    //These three must come after the 2 char operators since otherwise the 2 long ones would
    //never work
    if (t->data[t->curr] == '=') {
        PopChar(t);
        t->token = (Token){ .t = TOKEN_EQ };
        return;
    }

    if (t->data[t->curr] == '<') {
        PopChar(t);
        t->token = (Token){ .t = TOKEN_LT };
        return;
    }

    if (t->data[t->curr] == '>') {
        PopChar(t);
        t->token = (Token){ .t = TOKEN_GT };
        return;
    }

    if (isAlpha(t->data[t->curr])) {
        //ID
        char* start = &t->data[t->curr];
        u32 len = 0;
        while (isAlNum(t->data[t->curr]) && PopChar(t)) len++;

        SString data = {
            .data = (i8*)start,
            .len = len,
        };

        if (Sstrcmp(data, sstring("AND"))) {
            t->token = (Token){TOKEN_AND};
            return;
        }

        if (Sstrcmp(data, sstring("OR"))) {
            t->token = (Token){TOKEN_OR};
            return;
        }

        if (Sstrcmp(data, sstring("NOT"))) {
            t->token = (Token){TOKEN_NOT};
            return;
        }

        if (Sstrcmp(data, sstring("ANY"))) {
            t->token = (Token){TOKEN_ANY};
            return;
        }


        t->token = (Token){.t = TOKEN_ID, .token = data};
        return;
    }

    if (t->data[t->curr] == ':') {
        //tag
        PopChar(t);

        char* start = &t->data[t->curr];
        u32 len = 0;
        while (isAlNum(t->data[t->curr]) && PopChar(t)) len++;
        t->token = (Token){.t = TOKEN_TAG, .token = (SString){len, (i8*)start}};
        return;
    }

    if (isNum(t->data[t->curr])) {
        //NUM
        char* start = &t->data[t->curr];
        u32 len = 0;
        while (isNum(t->data[t->curr]) && PopChar(t)) len++;
        if (t->data[t->curr] == '.') {
            PopChar(t), len++;
            while (isNum(t->data[t->curr]) && PopChar(t)) len++;
        }

        t->token = (Token){TOKEN_NUM, .token = (SString){len, (i8*)start}};
        return;
    }

    if (t->data[t->curr] == '(') {
        PopChar(t);
        t->token = (Token){TOKEN_OPAREN};
        return;
    }

    if (t->data[t->curr] == ')') {
        PopChar(t);
        t->token = (Token){TOKEN_CPAREN};
        return;
    }

    PopChar(t);
    t->token = (Token){TOKEN_INVALID};
    return;
}


#define Expect(p, x) \
    do { \
        if (p->tok.token.t != x) {\
            p->err.code = ERROR_UNEXPECTED_TOKEN; \
            p->err.index = p->tok.curr; \
            longjmp(p->err.handler, 1); \
        } \
    } while (0)

#define CheckValid(p) \
    do { \
        if (p->tok.token.t == TOKEN_INVALID) {\
            p->err.code = ERROR_INVALID_TOKEN; \
            p->err.index = p->tok.curr; \
            longjmp(p->err.handler, 1); \
        } \
    } while (0)

#define NextToken(p) \
    do { \
        _NextToken(&(p)->tok); \
        CheckValid((p)); \
    } while (0)

INTERNAL_ONLY void ParseExpr(ParsingData* p);
INTERNAL_ONLY void ParseOExpr(ParsingData* p);
INTERNAL_ONLY void ParseCExpr(ParsingData* p);
INTERNAL_ONLY void ParseUnary(ParsingData* p, bool8 negated);
INTERNAL_ONLY void ParseID(ParsingData* p);

TagIter SearchWithExpr(TodoCtx* ctx, SString data, char** errmsg) {
    ParsingData p = {
        .tok = {
            .data = (char*)data.data,
            .max = data.len,
        },
        .query.a = GlobalAllocator
    };

    int r = setjmp(p.err.handler);
    if (r) {
        //handle error
        u32 len = errormsgs[p.err.code].len + 2 * token_names[p.tok.token.t].len + 2;
        char* msg = Alloc(GlobalAllocator, len);
        memset(msg, 0, len);
        sformat((SString){len, (i8*)msg}, (char*)errormsgs[p.err.code].data, token_names[p.tok.token.t], p.tok.token.token);

        *errmsg = msg;
        return (TagIter){0};
    }


    SString prefix = sstring(
            "SELECT todos.id, todos.name, todos.body, todos.priority AS PRIORITY, todos.completion AS COMPLETION "
            "FROM todos LEFT JOIN todo_tags ON todos.id = todo_tags.todo "
            "LEFT JOIN tags ON tags.id = todo_tags.tag "
            "GROUP BY todos.name "
            "HAVING ");

    dynExt(p.query, prefix.data, prefix.len);

    //run next token to init tokenizer
    NextToken(&p);
    ParseExpr(&p);

    //printlog("Query: %s\n", (SString){p.query.size, (i8*)p.query.data});

    TagIter output = {};
    SQL_CALL(sqlite3_prepare_v2(ctx->db, p.query.data, p.query.size, &output.query, NULL));

    dynFree(p.query);


    if (errmsg) {
        //mark as success{
        *errmsg = NULL;
    }
    return output;
}

INTERNAL_ONLY void ParseUnary(ParsingData* p, bool8 negated) {
    if (p->tok.token.t == TOKEN_NOT) {
        NextToken(p);
        ParseUnary(p, !negated);
        return;
    }

    if (p->tok.token.t == TOKEN_OPAREN) {
        dynPush(p->query, '(');

        NextToken(p);
        ParseExpr(p);

        dynPush(p->query, ')');

        Expect(p, TOKEN_CPAREN);

        NextToken(p);
        return;
    }

    if (p->tok.token.t == TOKEN_TAG) {
        dynExt(p->query, "SUM( IFNULL(tags.name = \'", 25);
        dynExt(p->query, p->tok.token.token.data, p->tok.token.token.len);
        dynExt(p->query, "\', 0) )", 7);

        if (negated) {
            dynExt(p->query, " = 0", 4);
        } else {
            dynExt(p->query, " > 0", 4);
        }
        NextToken(p);
        return;
    }

    if (p->tok.token.t == TOKEN_ANY) {

        if (negated) {
            dynExt(p->query, "false", 5);
        } else {
            dynExt(p->query, "true", 4);
        }
        NextToken(p);
        return;
    }

    p->err.code = ERROR_INVALID_UNARY;
    p->err.index = p->tok.curr;
    longjmp(p->err.handler, 1);
}

INTERNAL_ONLY void ParseID(ParsingData* p) {
    if (p->tok.token.t == TOKEN_NUM ||
        Sstrcmp(p->tok.token.token, sstring("PRIORITY")) ||
        Sstrcmp(p->tok.token.token, sstring("COMPLETION"))) {

        dynExt(p->query, p->tok.token.token.data, p->tok.token.token.len);
        NextToken(p); 
        return;    
    }

    dynExt(p->query, "(todos.name LIKE \'%", 19);
    dynExt(p->query, p->tok.token.token.data, p->tok.token.token.len);
    dynExt(p->query, "%\')", 3);

    NextToken(p); 
}

INTERNAL_ONLY void ParseCExpr(ParsingData* p) {
    ParseID(p);

    if (p->tok.token.t == TOKEN_OR) return; //done parsing
    if (p->tok.token.t == TOKEN_AND) return; //done parsing
    if (p->tok.token.t == TOKEN_END) return; //done parsing
    if (p->tok.token.t == TOKEN_CPAREN) return;

    if (p->tok.token.t == TOKEN_EQ) dynExt(p->query, " = ", 3);
    if (p->tok.token.t == TOKEN_LT) dynExt(p->query, " < ", 3);
    if (p->tok.token.t == TOKEN_GT) dynExt(p->query, " > ", 3);
    if (p->tok.token.t == TOKEN_NEQ) dynExt(p->query, " != ", 4);
    if (p->tok.token.t == TOKEN_LE) dynExt(p->query, " <= ", 4);
    if (p->tok.token.t == TOKEN_GE) dynExt(p->query, " >= ", 4);

    NextToken(p); 
    ParseCExpr(p);
}

INTERNAL_ONLY void ParseOExpr(ParsingData* p) {

    if (p->tok.token.t == TOKEN_OPAREN || p->tok.token.t == TOKEN_ANY || p->tok.token.t == TOKEN_TAG || p->tok.token.t == TOKEN_NOT)
        ParseUnary(p, FALSE);
    else { 
        dynExt(p->query, "SUM( IFNULL(", 12);
        ParseCExpr(p);
        dynExt(p->query, ", 0) )", 6);
        dynExt(p->query, " > 0", 4);
    }

    if (p->tok.token.t == TOKEN_AND) return; //done parsing
    if (p->tok.token.t == TOKEN_END) return; //done parsing
    if (p->tok.token.t == TOKEN_CPAREN) return;
    //NextToken(&p->tok); 

    Expect(p, TOKEN_OR);

    dynExt(p->query, " OR ", 4);

    NextToken(p); 
    ParseOExpr(p);
}

INTERNAL_ONLY void ParseExpr(ParsingData* p) {
    ParseOExpr(p); //always start with Unary

    if (p->tok.token.t == TOKEN_END) return; //done parsing
    if (p->tok.token.t == TOKEN_CPAREN) return;
    //NextToken(&p->tok); 

    Expect(p, TOKEN_AND);

    dynExt(p->query, " AND ", 5);
    NextToken(p); 
    ParseExpr(p);
}



//This function is idempotent and sets up
//the database structure. This should be
//called on startup and when resetting the 
//database
static void InitializeTables(sqlite3* db) {
    char* err;
    int rc = sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, &err);

    if (rc) {
        printerr("SQLITE Error: %n\n", err);
        panic();
    }

    //init table schema
    rc = sqlite3_exec(db, 
            "CREATE TABLE IF NOT EXISTS todos"
            "("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name STRING UNIQUE,"
            "body STRING,"
            "priority FLOAT, "
            "completion INTEGER CHECK( completion <= 100 )"
            ")",
            NULL, NULL, &err);

    if (rc) {
        printerr("SQLITE Error: %n\n", err);
        panic();
    }

    rc = sqlite3_exec(db, 
            "CREATE TABLE IF NOT EXISTS tags"
            "("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name STRING UNIQUE,"
            "hits INTEGER"
            ")",
            NULL, NULL, &err);

    if (rc) {
        printerr("SQLITE Error: %n\n", err);
        panic();
    }

    rc = sqlite3_exec(db, 
            "CREATE TABLE IF NOT EXISTS todo_tags"
            "("
            "todo,"
            "tag,"
            "FOREIGN KEY(todo) REFERENCES todos(id) ON DELETE CASCADE,"
            "FOREIGN KEY(tag) REFERENCES tags(id) ON DELETE CASCADE,"
            "UNIQUE (todo, tag) "
            ")",
            NULL, NULL, &err);

    if (rc) {
        printerr("SQLITE Error: %n\n", err);
        panic();
    }

    rc = sqlite3_exec(db, 
            "CREATE UNIQUE INDEX IF NOT EXISTS "
            "todo_ids ON todos(id);",
            NULL, NULL, &err);

    if (rc) {
        printerr("SQLITE Error: %n\n", err);
        panic();
    }

    rc = sqlite3_exec(db, 
            "CREATE UNIQUE INDEX IF NOT EXISTS "
            "tag_ids ON tags(id);",
            NULL, NULL, &err);

    if (rc) {
        printerr("SQLITE Error: %n\n", err);
        panic();
    }
}

SString default_config = sstring("db_loc: ~/.local/share/simpletodo/database");

TodoCtx InitDataBase(SString overrides) {
    TodoCtx out = { };
    char* err;

    out.mem = ArenaCreate(GlobalAllocator, KB(10));
    out.tag_ids.a = ArenaGetAllocator(out.mem);

    ConfigMap config = { .a = GlobalAllocator };
    ParseConfig(&config, default_config);
    ParseConfig(&config, overrides);
    GetConfig(&config);

    //printlog("Db_loc: %s\n", dynBack((ConfigMapGet(&config, sstring("db_loc"))->vals)));

    out.db_loc = dynBack((ConfigMapGet(&config, sstring("db_loc"))->vals));

    //mkdir
    assert(!mkdirRec(filepathpopdir(out.db_loc)));
    assert(!sqlite3_open((char*)out.db_loc.data, &out.db));
    InitializeTables(out.db);
    
    ConfigVal* v = ConfigMapGet(&config, sstring("addkey")); 
    if (v) {
        for (u32 i = 0; i < v->vals.size; i++) {
            u64 id = AddTag(&out, v->vals.data[i]);
            dynPush(out.tag_ids, id);
        }
    }
    out.config = config;

    return out;
}

void ClearDataBase(TodoCtx* ctx) {
    sqlite3_db_config(ctx->db, SQLITE_DBCONFIG_RESET_DATABASE, 1, 0);
    sqlite3_exec(ctx->db, "VACUUM", NULL, NULL, NULL);
    sqlite3_db_config(ctx->db, SQLITE_DBCONFIG_RESET_DATABASE, 0, 0);

    InitializeTables(ctx->db); //reinitialize tables

    for (u32 i = 0; i < ctx->default_tags.size; i++) {
        u64 id = AddTag(ctx, ctx->default_tags.data[i]);
    }
}

void DeleteTodoID(TodoCtx* ctx, u64 t) {
    static sqlite3_stmt* statement = NULL;

    DeleteLinkTodo(ctx, t);
    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "DELETE FROM todos WHERE rowid = ?", -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_int64(statement, 1, t));

    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) continue;
        if (rc == SQLITE_DONE) return; //success
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();
}

void DeleteTodoName(TodoCtx* ctx, SString name) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "DELETE FROM todos WHERE name = ?", -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_text(statement, 1, (char*)name.data, name.len, SQLITE_TRANSIENT));

    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) continue;
        if (rc == SQLITE_DONE) return; //success
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();

}

u64 AddTodo(TodoCtx* ctx, SString name, SString body, f32 priority, u32 completion) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "INSERT INTO todos(id, name, body, priority, completion) VALUES(NULL, ?, ?, ?, ?) ON CONFLICT(name) DO UPDATE SET body = excluded.body, priority = excluded.priority, completion = excluded.completion RETURNING id", -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_text(statement, 1, (char*)name.data, name.len, SQLITE_TRANSIENT));
    SQL_CALL(sqlite3_bind_text(statement, 2, (char*)body.data, body.len, SQLITE_TRANSIENT));
    SQL_CALL(sqlite3_bind_double(statement, 3, priority));
    SQL_CALL(sqlite3_bind_int64(statement, 4, completion));

    int rc;
    i64 id;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) {
            id = sqlite3_column_int64(statement, 0);
            continue;
        }
        if (rc == SQLITE_DONE) {

            for (u32 i = 0; i < ctx->tag_ids.size; i++) {
                //add required tags
                AddLink(ctx, id, ctx->tag_ids.data[i]);
            }

            return id; //success
        }
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();
}

Todo GetTodoID(TodoCtx* ctx, Allocator a, u64 id) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        //we already know the id
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "SELECT name, body, priority, completion FROM todos WHERE rowid = ?" , -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_int64(statement, 1, id));

    Todo res = {.id = id};
    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) {
            {
                const u8* data = sqlite3_column_text(statement, 0);
                u32 size = sqlite3_column_bytes(statement, 0);
                res.name = Sstrdup(a, (SString){.data = (i8*)data, .len = size});
            }
            {
                const u8* data = sqlite3_column_text(statement, 1);
                u32 size = sqlite3_column_bytes(statement, 1);
                res.body = Sstrdup(a, (SString){.data = (i8*)data, .len = size});
            }
            {
                res.priority = sqlite3_column_double(statement, 2);
                res.completion = sqlite3_column_int(statement, 3);
            }
            continue;
        }
        if (rc == SQLITE_DONE) {
            return res; //success
        }
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();
}

Todo GetTodoName(TodoCtx* ctx, Allocator a, SString name) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        //we already know the id
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "SELECT id, body, priority, completion FROM todos WHERE name = ?" , -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_text(statement, 1, (char*)name.data, name.len, SQLITE_TRANSIENT));

    Todo res = {.name = Sstrdup(a, name)};
    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) {
            {
                res.id = sqlite3_column_int64(statement, 0);
            }
            {
                const u8* data = sqlite3_column_text(statement, 1);
                u32 size = sqlite3_column_bytes(statement, 1);
                res.body = Sstrdup(a, (SString){.data = (i8*)data, .len = size});
            }
            {
                res.priority = sqlite3_column_double(statement, 2);
                res.completion = sqlite3_column_int(statement, 3);
            }
            continue;
        }
        if (rc == SQLITE_DONE) {
            return res; //success
        }
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();
}

SString StringifyTodo(TodoCtx* ctx, Todo* t, ArenaAllocator a) {
    char* buffer = ArenaAllocZero(a, 0);
    u32 curr = 0;

    //title
    ArenaExtendLast(a, 2);
    buffer[curr++] = '#';
    buffer[curr++] = ' ';

    ArenaExtendLast(a, t->name.len + 1);
    for (u32 i = 0; i < t->name.len; i++) {
        buffer[curr++] = t->name.data[i];
    }
    buffer[curr++] = '\n';

    //status
    SString status_prefix = sstring("Completion: ");
    ArenaExtendLast(a, status_prefix.len + 15);
    curr += sformat((SString){.data = (i8*)&buffer[curr], .len = status_prefix.len + 15}, "%s%d%%", status_prefix, t->completion);
    buffer[curr++] = '\n';

    //priority
    SString priority_prefix = sstring("Priority: ");
    ArenaExtendLast(a, priority_prefix.len + 15);
    curr += sformat((SString){.data = (i8*)&buffer[curr], .len = priority_prefix.len + 15}, "%s%f", priority_prefix, t->priority);
    buffer[curr++] = '\n';

    //tags
    TodoIter it = BeginTodoIter(ctx, t->id);
    
    while (1) {
        ScratchArena sc = ScratchArenaGet(a);
        Tag tag;
        IterState s = NextTag(&it, ArenaGetAllocator(sc.arena), &tag);
        if (s == ITER_DONE || s == ITER_ERR) {
            ScratchArenaEnd(sc);
            break;
        }

        ArenaExtendLast(a, tag.name.len + 2);
        buffer[curr++] = ':';
        for (u32 i = 0; i < tag.name.len; i++) {
            buffer[curr++] = tag.name.data[i];
        }
        buffer[curr++] = ' ';

        ScratchArenaEnd(sc);
    }

    ArenaExtendLast(a, 2);
    buffer[curr++] = '\n';
    buffer[curr++] = '\n';

    ArenaExtendLast(a, t->body.len);
    for (u32 i = 0; i < t->body.len; i++) {
        buffer[curr++] = t->body.data[i];
    }

    return (SString){curr, (i8*)buffer};
}

#define Advance(c, lim) \
    do { \
        c++; \
        if (c >= lim) { \
            *err = "Unexpected End of Data"; \
            return (Todo){0}; \
        } \
    } while(0)

//this function sanitizes user input so it must be checked
Todo DeStringifyTodo(TodoCtx* ctx, SString data, Allocator a, char** err) {
    char* tmp;
    if (err == NULL) {
        err = &tmp; //avoid branching logic and handle null error
    }

    if (data.len < 3) {
        *err = "Name must be preceeded by '# ' for valid markdown";
        return (Todo){0};
    }

    //get name 
    u32 curr = 2;
    SString name = {
        .data = &data.data[2]
    };

    while (data.data[curr] != '\n' && curr < data.len) {
        name.len++;
        curr++;
    }
    if (name.len == 0) {
        *err = "Name Cannot Be Empty";
        return (Todo){0};
    }

    {
        //DO NOT call allocator a in here,
        //this uses the scratch allocator and
        //calling a in here can cause memory corruption

        ScratchArena sc = ScratchArenaGet(NULL);
        Allocator scratch = ArenaGetAllocator(sc.arena);
        //this clears the tags so that we can remove tags

        Todo t = GetTodoName(ctx, scratch, name);
        DeleteLinkTodo(ctx, t.id);
        ScratchArenaEnd(sc);
    }

    u64 todo_id = AddTodo(ctx, name, sstring(""), 0, 0);

    if (curr >= data.len) {
        return GetTodoID(ctx, a, todo_id);
    }
    Advance(curr, data.len);

    //completion
    SString status_prefix = sstring("Completion: ");
    for (u32 i = 0; i < status_prefix.len; i++) {
        if (status_prefix.data[i] != data.data[curr]) {
            return GetTodoID(ctx, a, todo_id);
        }
        Advance(curr, data.len);
    }

    u32 completion;
    curr += StrtoU32((SString){.data = &data.data[curr], .len = data.len - curr}, &completion);
    while (data.data[curr] != '\n') Advance(curr, data.len);

    if (curr > data.len) {
        u64 todo_id = AddTodo(ctx, name, sstring(""), 0, completion);
        return GetTodoID(ctx, a, todo_id);
    }
    Advance(curr, data.len);
    
    //priority
    SString priority_prefix = sstring("Priority: ");
    for (u32 i = 0; i < priority_prefix.len; i++) {
        if (priority_prefix.data[i] != data.data[curr++]) {
            return GetTodoID(ctx, a, todo_id);
        }
    }
    
    f32 priority;
    curr += Strtofloat((SString){.data = &data.data[curr], .len = data.len - curr}, &priority);
    while (data.data[curr] != '\n') Advance(curr, data.len);
    if (curr > data.len) {
        u64 todo_id = AddTodo(ctx, name, sstring(""), priority, completion);
        return GetTodoID(ctx, a, todo_id);
    }
    Advance(curr, data.len);

    //get tags
    SString curr_tag = {0};
    while (1) {
        if (data.data[curr] == ' ' || data.data[curr] == '\n') {
            if (curr_tag.data) {
                curr_tag.len = &data.data[curr] - curr_tag.data;

                u64 tag_id = AddTag(ctx, curr_tag);
                AddLink(ctx, todo_id, tag_id);
            }

            curr_tag.data = NULL;
            if (data.data[curr] == '\n') break;
        }

        if (data.data[curr] == ':') {
            curr_tag.data = &data.data[curr + 1];

        }

        Advance(curr, data.len); 
    }

    //expecting newlines
    while (curr <= data.len && data.data[curr] == '\n') curr++; 
    if (curr > data.len) {
        //empty body
        return GetTodoID(ctx, a, todo_id);
    }

    //pull body
    SString body = { .data = &data.data[curr] };
    while (curr < data.len && data.data[curr] != 0) body.len++, curr++;

    *err = NULL;
    AddTodo(ctx, name, body, priority, completion);
    return GetTodoID(ctx, a, todo_id);
}

TodoIter BeginTodoIter(TodoCtx* ctx, u64 todo) {
    sqlite3_stmt* statement = NULL;

    //we already know the id
    SQL_CALL(sqlite3_prepare_v2(ctx->db, 
                "SELECT id, name FROM "
                "todo_tags JOIN tags ON "
                "todo_tags.tag = tags.id "
                "WHERE todo_tags.todo = ? "
                "GROUP BY tags.id",
                -1, &statement, NULL));

    SQL_CALL(sqlite3_bind_int64(statement, 1, todo));

    return (TodoIter){statement};
}

void ResetTodoIter(TodoIter* todo) {
    sqlite3_reset(todo->query);
}

IterState NextTag(TodoIter* iter, Allocator a, Tag* out) {

    while (1) {
        int rc = sqlite3_step(iter->query);
        if (rc == SQLITE_BUSY) continue;
        if (rc == SQLITE_DONE) {
            return ITER_DONE;
        }
        if (rc == SQLITE_ROW) {
            out->id = sqlite3_column_int64(iter->query, 0);
            SString data = {
                .data = (i8*)sqlite3_column_text(iter->query, 1),
                .len = sqlite3_column_bytes(iter->query, 1)
            };
            out->name = Sstrdup(a, data);

            return ITER_PENDING;
        }

        printlog("Iter Error: %n\n", sqlite3_errstr(rc));
        return ITER_ERR;
    }
}

void EndTodoIter(TodoIter* iter) {
    sqlite3_finalize(iter->query);
}



void DeleteTagID(TodoCtx* ctx, u64 id) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "DELETE FROM tags WHERE rowid = ?" , -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_int64(statement, 1, id));

    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) continue;
        if (rc == SQLITE_DONE) return; //success
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();
}

void DeleteTagName(TodoCtx* ctx, SString name) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "DELETE FROM tags WHERE name = ?", -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_text(statement, 1, (char*)name.data, name.len, SQLITE_TRANSIENT));

    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) continue;
        if (rc == SQLITE_DONE) return; //success
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();
}

u64 AddTag(TodoCtx* ctx, SString name) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "INSERT INTO tags(id, name, hits) VALUES(NULL, ?, 0) ON CONFLICT(name) DO UPDATE SET hits = hits + 1 RETURNING id", -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_text(statement, 1, (char*)name.data, name.len, SQLITE_TRANSIENT));

    int rc;
    i64 id = 0;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) {
            id = sqlite3_column_int64(statement, 0);
            continue;
        }
        if (rc == SQLITE_DONE) {
            return id; //success
        }
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    printlog("SQLITE ERROR: %n\n", sqlite3_errstr(rc));
    panic();
}

Tag GetTagID(TodoCtx* ctx, Allocator a, u64 id) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        //we already know the id
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "SELECT name FROM tags WHERE rowid = ?" , -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_int64(statement, 1, id));

    Tag res = {.id = id};
    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) {
            const u8* data = sqlite3_column_text(statement, 0);
            u32 size = sqlite3_column_bytes(statement, 0);
            res.name = Sstrdup(a, (SString){.data = (i8*)data, .len = size});
            continue;
        }
        if (rc == SQLITE_DONE) {
            return res; //success
        }
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();
}

Tag GetTagName(TodoCtx* ctx, Allocator a, SString name) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        //we already know the id
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "SELECT id FROM tags WHERE name = ?" , -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_text(statement, 1, (char*)name.data, name.len, SQLITE_TRANSIENT));

    Tag res = {.name = Sstrdup(a, name) };
    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) {
            res.id = sqlite3_column_int64(statement, 0);
            continue;
        }
        if (rc == SQLITE_DONE) {
            return res; //success
        }
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();



}

TagIter BeginTagIter(TodoCtx* ctx, u64 tag) {
    sqlite3_stmt* statement = NULL;

    //we already know the id
    SQL_CALL(sqlite3_prepare_v2(ctx->db, 
                "SELECT id, name, body, priority FROM "
                "todo_tags JOIN todos ON "
                "todo_tags.todo = todos.id "
                "WHERE todo_tags.tag = ? "
                "ORDER BY priority DESC",
                -1, &statement, NULL));

    SQL_CALL(sqlite3_bind_int64(statement, 1, tag));

    return (TagIter){statement};
}


void ResetTagIter(TagIter* todo) {
    sqlite3_reset(todo->query);
}

IterState NextTodo(TagIter* iter, Allocator a, Todo* out) {
    while (1) {
        int rc = sqlite3_step(iter->query);
        if (rc == SQLITE_BUSY) continue;
        if (rc == SQLITE_DONE) {
            return ITER_DONE;
        }
        if (rc == SQLITE_ROW) {
            out->id = sqlite3_column_int64(iter->query, 0);
            SString data = {
                .data = (i8*)sqlite3_column_text(iter->query, 1),
                .len = sqlite3_column_bytes(iter->query, 1)
            };
            out->name = Sstrdup(a, data);

            data = (SString) {
                .data = (i8*)sqlite3_column_text(iter->query, 2),
                .len = sqlite3_column_bytes(iter->query, 2)
            };

            out->body = Sstrdup(a, data);
            out->priority = sqlite3_column_double(iter->query, 3);

            return ITER_PENDING;
        }

        printlog("Iter Error: %n\n", sqlite3_errstr(rc));
        return ITER_ERR;
    }
}

void EndTagIter(TagIter* iter) {
    sqlite3_finalize(iter->query);
}


void DeleteLinkID(TodoCtx* ctx, u64 todo, u64 tag) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "DELETE FROM todo_tags WHERE todo = ? AND tag = ?", -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_int64(statement, 1, todo));
    SQL_CALL(sqlite3_bind_int64(statement, 2, tag));

    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) continue;
        if (rc == SQLITE_DONE) return; //success
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();
}

void DeleteLinkTodo(TodoCtx* ctx, u64 todo) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "DELETE FROM todo_tags WHERE todo = ?", -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_int64(statement, 1, todo));

    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) continue;
        if (rc == SQLITE_DONE) return; //success
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();
}

void DeleteLinkTag(TodoCtx* ctx, u64 tag) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "DELETE FROM todo_tags WHERE tag = ?", -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_int64(statement, 1, tag));

    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) continue;
        if (rc == SQLITE_DONE) return; //success
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();
}

void AddLink(TodoCtx* ctx, u64 todo, u64 tag) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "INSERT INTO todo_tags(todo, tag) VALUES(?, ?) ON CONFLICT(todo, tag) DO NOTHING", -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_int64(statement, 1, todo));
    SQL_CALL(sqlite3_bind_int64(statement, 2, tag));

    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_DONE) {
            return; //success
        }
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    printlog("SQLITE ERROR: %n\n", sqlite3_errstr(rc));
    panic();
}

bool8 GetLink(TodoCtx* ctx, u64 todo, u64 tag) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        //we already know the id
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "SELECT * FROM todo_tags WHERE todo = ? AND tag = ?" , -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_int64(statement, 1, todo));
    SQL_CALL(sqlite3_bind_int64(statement, 2, tag));

    bool8 res = FALSE;
    int rc;
    while (1) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) {
            res = TRUE;
            continue;
        }
        if (rc == SQLITE_DONE) {
            return res; //success
        }
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();
}

#include "limits.h"
#include "unistd.h"

void GetConfig(ConfigMap* out) {
    //save current dir
    
    char buffer[PATH_MAX + 1] = {0};
    char* r = getcwd(buffer, PATH_MAX);
    if (!r) panic();

    //test walk up directories    
    u32 max_depth = 5;
    while (max_depth) {
        file f = filestat(sstring(".todo.conf"));
        if (f.stats.size) {
            ScratchArena sc = ScratchArenaGet(NULL);
            SString config = {
                .data = ArenaAllocZero(sc.arena, f.stats.size),
                .len = f.stats.size
            };
            fileload(config, sstring(".todo.conf"));
            //printlog("Found Config\n");
            //printlog("%s\n", config);
            ParseConfig(out, config) ;

            ScratchArenaEnd(sc);
        }
        setdir(sstring("..")); //walk up directory
        max_depth--;
    }


    setdir((SString){.len = strlen(buffer), .data = (i8*)buffer});
}

void ParseConfig(ConfigMap* m, SString config) {
    u32 curr = 0;
    SString key = {.data = config.data};
    SString val = {};

    //FALSE => key
    //TRUE => val
    bool8 parse_state = TRUE;

    while (curr < config.len) {
        i8 c = config.data[curr];
        if (parse_state) {
            if (c == ':') {
                parse_state = FALSE;
                curr += 2; //skip ': '
                val.data = &config.data[curr];
                val.len = 0;
                continue;
            }
            key.len++;
            curr++;
        } else {
            if (c == '\n' || curr == config.len - 1) {
                parse_state = TRUE;

                if (c != '\n') val.len++;
                ConfigVal* v = ConfigMapGet(m, key);
                if (!v) {
                    ConfigVal n = {
                        .vals.a = m->a,
                        .mem.a = m->a,
                    };
                    ConfigMapIns(m, key, n); 
                    v = ConfigMapGet(m, key);
                }

                void* dst = DynArenaAlloc(&v->mem, val.len);
                memcpy(dst, val.data, val.len);
                val.data = dst;
                dynPush(v->vals, val);

                curr++;
                key.data = &config.data[curr];
                key.len = 0;
                continue;
            }
            val.len++;
            curr++;
        } 
    }
}
