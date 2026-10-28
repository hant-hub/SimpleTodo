#include <core.h>


typedef enum TokenType {
    TOKEN_INVALID = 0, //uninitialized tokens are invalid
    TOKEN_ID,
    TOKEN_NUM,
    TOKEN_TAG,
    TOKEN_AND,
    TOKEN_NOT,
    TOKEN_OR,
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
    [TOKEN_OR]      = sstring("OR"),
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
// OEXPR := UNARY
//       |  UNARY OR OEXPR
//
// UNARY := NOT UNARY
//        | TAG
//        | ID
//        | '(' EXPR ')'
//
// TAG := ':' ID
//

static void NextToken(ExprTokenizer* t) {
    //Eat Whitespace
    while (isSpace(t->data[t->curr]) && PopChar(t));

    if (t->curr >= t->max) {
        t->token = (Token){TOKEN_END};
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

    t->token = (Token){TOKEN_INVALID};
    return;
}

#include "setjmp.h"

typedef enum ErrorCode {
    ERROR_NO_ERROR = 0,
    ERROR_INVALID_UNARY,
    ERROR_INVALID_ID,
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

#define Expect(p, x) \
    do { \
        if (p->tok.token.t != x) {\
            p->err.code = ERROR_UNEXPECTED_TOKEN; \
            p->err.index = p->tok.curr; \
            longjmp(p->err.handler, 1); \
        } \
    } while (0)

const SString errormsgs[] = {
    [ERROR_NO_ERROR]            = sstring("No Error: Should never happen"),
    [ERROR_INVALID_UNARY]       = sstring("Invalid Unary: Expected TAG, NOT, or '(' Got %s"),
    [ERROR_INVALID_ID]          = sstring("Invalid %s: Got '%s'"),
    [ERROR_UNEXPECTED_TOKEN]    = sstring("Unexpected Token: Got %s"),
};

static void ParseExpr(ParsingData* p);
static void ParseOExpr(ParsingData* p);
static void ParseUnary(ParsingData* p, bool8 negated);

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
        u32 len = errormsgs[p.err.code].len + token_names[p.tok.token.t].len;
        char* msg = Alloc(GlobalAllocator, len);
        sformat((SString){len, (i8*)msg}, (char*)errormsgs[p.err.code].data, token_names[p.tok.token.t], p.tok.token.token);

        *errmsg = msg;
        return (TagIter){0};
    }


    SString prefix = sstring(
        "SELECT todos.id, todos.name, todos.body FROM todos LEFT JOIN todo_tags ON todos.id = todo_tags.todo "
            "LEFT JOIN tags ON tags.id = todo_tags.tag "
            "GROUP BY todos.name "
            "HAVING ");

    dynExt(p.query, prefix.data, prefix.len);

    //run next token to init tokenizer
    NextToken(&p.tok);
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

static void ParseUnary(ParsingData* p, bool8 negated) {
    if (p->tok.token.t == TOKEN_NOT) {
        NextToken(&p->tok);
        ParseUnary(p, !negated);
        return;
    }

    if (p->tok.token.t == TOKEN_OPAREN) {
        dynPush(p->query, '(');

        NextToken(&p->tok);
        ParseExpr(p);

        dynPush(p->query, ')');

        Expect(p, TOKEN_CPAREN);

        NextToken(&p->tok);
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
        NextToken(&p->tok);
        return;
    }

    if (p->tok.token.t == TOKEN_ID) {
        p->err.code = ERROR_INVALID_ID;
        p->err.index = p->tok.curr;
        longjmp(p->err.handler, 1);

        NextToken(&p->tok);
        return;
    }


    p->err.code = ERROR_INVALID_UNARY;
    p->err.index = p->tok.curr;
    longjmp(p->err.handler, 1);
}

static void ParseOExpr(ParsingData* p) {
    ParseUnary(p, FALSE); //always start with Unary

    if (p->tok.token.t == TOKEN_AND) return; //done parsing
    if (p->tok.token.t == TOKEN_END) return; //done parsing
    if (p->tok.token.t == TOKEN_CPAREN) return;
    //NextToken(&p->tok); 

    Expect(p, TOKEN_OR);

    dynExt(p->query, " OR ", 4);

    NextToken(&p->tok); 
    ParseExpr(p);
}

static void ParseExpr(ParsingData* p) {
    ParseOExpr(p); //always start with Unary

    if (p->tok.token.t == TOKEN_END) return; //done parsing
    if (p->tok.token.t == TOKEN_CPAREN) return;
    //NextToken(&p->tok); 

    Expect(p, TOKEN_AND);

    dynExt(p->query, " AND ", 5);
    NextToken(&p->tok); 
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
            "body STRING"
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
}

TodoCtx InitDataBase() {
    TodoCtx out = { };
    char* err;

    out.mem = ArenaCreate(GlobalAllocator, KB(10));
    out.tag_ids.a = ArenaGetAllocator(out.mem);

    Config c = GetConfig(ArenaGetAllocator(out.mem));

    assert(!sqlite3_open((char*)c.db_loc.data, &out.db));
    InitializeTables(out.db);

    for (u32 i = 0; i < c.default_tags.size; i++) {
        u64 id = AddTag(&out, c.default_tags.data[i]);
        dynPush(out.tag_ids, id);
    }

    return out;
}

void ClearDataBase(TodoCtx* ctx) {
    sqlite3_db_config(ctx->db, SQLITE_DBCONFIG_RESET_DATABASE, 1, 0);
    sqlite3_exec(ctx->db, "VACUUM", NULL, NULL, NULL);
    sqlite3_db_config(ctx->db, SQLITE_DBCONFIG_RESET_DATABASE, 0, 0);

    InitializeTables(ctx->db); //reinitialize tables
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

u64 AddTodo(TodoCtx* ctx, SString name, SString body) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "INSERT INTO todos(id, name, body) VALUES(NULL, ?, ?) ON CONFLICT(name) DO UPDATE SET body = excluded.body RETURNING id", -1, &statement, NULL));
    } else {
        sqlite3_reset(statement);
    }

    SQL_CALL(sqlite3_bind_text(statement, 1, (char*)name.data, name.len, SQLITE_TRANSIENT));
    SQL_CALL(sqlite3_bind_text(statement, 2, (char*)body.data, body.len, SQLITE_TRANSIENT));

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
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "SELECT name, body FROM todos WHERE rowid = ?" , -1, &statement, NULL));
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
        SQL_CALL(sqlite3_prepare_v2(ctx->db, "SELECT id, body FROM todos WHERE name = ?" , -1, &statement, NULL));
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


TodoIter BeginTodoIter(TodoCtx* ctx, u64 todo) {
    sqlite3_stmt* statement = NULL;

    //we already know the id
    SQL_CALL(sqlite3_prepare_v2(ctx->db, 
                "SELECT id, name FROM "
                "todo_tags JOIN tags ON "
                "todo_tags.tag = tags.id "
                "WHERE todo_tags.todo = ?",
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
                "SELECT id, name, body FROM "
                "todo_tags JOIN todos ON "
                "todo_tags.todo = todos.id "
                "WHERE todo_tags.tag = ?",
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

Config GetConfig(Allocator a) {
    //save current dir
    
    char buffer[PATH_MAX + 1] = {0};
    getcwd(buffer, PATH_MAX);
    printlog("%n\n", buffer);

    //test walk up directories    
    
    u32 max_depth = 5;
    while (max_depth) {
        file f = filestat(sstring("simpletodo.conf"));
        if (f.stats.size) {
            break;
        }
        setdir(sstring("..")); //walk up directory
        max_depth--;
    }

    Config out = { .db_loc = sstring("~/.local/simpletodo/database"), .default_tags.a = a };
    if (!max_depth) {
        //default config
        return out;
    }

    //read configuration
    file config_file = fileopen(sstring("simpletodo.conf"), FILE_READ);

    ScratchArena sc = ScratchArenaGet(NULL);
    SString config_data = {
        .data = ArenaAlloc(sc.arena, config_file.stats.size),
        .len = config_file.stats.size,
    };

    fileread(config_data, config_file);
    fileclose(config_file);

    //db_loc = 
    printlog("Loaded Config:\n%s\n", config_data);
    
    i8* cur = (i8*)config_data.data;

    while (1) {
        SString key = { .data = cur, };
        //get key
        while ((cur - config_data.data <= config_data.len) && *cur != ' ') key.len++, cur++;        
        if (cur - config_data.data > config_data.len) {
            break;
        }

        cur++; //skip ' '
        
        //get val
        SString val = { .data = cur };
        while ((cur - config_data.data <= config_data.len) && *cur != '\n') val.len++, cur++;        
        if (cur - config_data.data > config_data.len) {
            break;
        }

        cur++; //skip \n

        printlog("\t%s %s\n", key, val);

        if (Sstrcmp(key, sstring("db_loc"))) {
            //manually to add null terminator
            out.db_loc.data = Alloc(a, key.len + 1); 
            out.db_loc.len = key.len;

            memset(out.db_loc.data, 0, key.len + 1);
            memcpy(out.db_loc.data, key.data, key.len); 
            continue;
        }

        if (Sstrcmp(key, sstring("addkey"))) {
            SString d = Sstrdup(a, val);
            dynPush(out.default_tags, d);
            continue;
        }
    }

    ScratchArenaEnd(sc);
    setdir((SString){.len = strlen(buffer), .data = (i8*)buffer});
    return out;
}



