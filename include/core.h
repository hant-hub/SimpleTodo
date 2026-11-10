#ifndef CORE_H
#define CORE_H

#include <core/cutils.h>
#include <ds/ds.h>
#include <sqlite3.h>

//Internal functions only
#ifdef DEBUG
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

static const SString token_names[] = {
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

void _NextToken(ExprTokenizer* t);

void ParseExpr(ParsingData* p);
void ParseOExpr(ParsingData* p);
void ParseCExpr(ParsingData* p);
void ParseUnary(ParsingData* p, bool8 negated);
void ParseID(ParsingData* p);
#endif


#define SQL_CALL(x) \
        do { \
            int rc = x; \
            if (rc) { \
                debugerr("SQL ERROR: %n", sqlite3_errstr(rc)); \
                panic(); \
            } \
        } while (0)

typedef struct Todo {
    u64 id;
    f32 priority;
    u32 completion;
    SString name;
    SString body;
} Todo;

typedef struct Tag {
    u64 id;
    SString name;
} Tag;

typedef struct TagIter {
    sqlite3_stmt* query;
} TagIter;

typedef struct TodoIter {
    sqlite3_stmt* query;
} TodoIter;

typedef struct Config {
    SString db_loc;
    dynArray(SString) default_tags;
} Config;

typedef struct TodoCtx {
    ArenaAllocator mem;
    sqlite3* db;
    Config c; 
    dynArray(u64) tag_ids;
} TodoCtx;

TodoCtx InitDataBase(Config overrides);
void ClearDataBase(TodoCtx* ctx);

//Todo
void DeleteTodoID(TodoCtx* ctx, u64 t);
void DeleteTodoName(TodoCtx* ctx, SString name);

u64 AddTodo(TodoCtx* ctx, SString name, SString body, f32 priority, u32 completion);
Todo GetTodoID(TodoCtx* ctx, Allocator a, u64 id);
Todo GetTodoName(TodoCtx* ctx, Allocator a, SString name);

//used for converting todos into human readable formats
SString StringifyTodo(TodoCtx* ctx, Todo* t, ArenaAllocator a); //should always succeed
                                                                //We can assume this since we sanitize
                                                                //the input before we input it into
                                                                //the database

Todo DeStringifyTodo(TodoCtx* ctx, SString data, Allocator a, char** err);

typedef enum IterState {
    ITER_DONE,
    ITER_PENDING,
    ITER_ERR,
} IterState;

TodoIter BeginTodoIter(TodoCtx* ctx, u64 todo);
void ResetTodoIter(TodoIter* todo);
IterState NextTag(TodoIter* iter, Allocator a, Tag* out);
void EndTodoIter(TodoIter* iter);

//Tag

void DeleteTagID(TodoCtx* ctx, u64 id);
void DeleteTagName(TodoCtx* ctx, SString name);

u64 AddTag(TodoCtx* ctx, SString name);
Tag GetTagID(TodoCtx* ctx, Allocator a, u64 id);
Tag GetTagName(TodoCtx* ctx, Allocator a, SString name);

TagIter BeginTagIter(TodoCtx* ctx, u64 tag);
void ResetTagIter(TagIter* todo);
IterState NextTodo(TagIter* iter, Allocator a, Todo* out);
void EndTagIter(TagIter* iter);


//Link
void DeleteLinkID(TodoCtx* ctx, u64 todo, u64 tag);
void DeleteLinkTodo(TodoCtx* ctx, u64 todo);
void DeleteLinkTag(TodoCtx* ctx, u64 tag);

void AddLink(TodoCtx* ctx, u64 todo, u64 tag);
bool8 GetLink(TodoCtx* ctx, u64 todo, u64 tag);


//Search

//Note(ELI): This function avoid needing to sanitize the user input by
//parsing it. Invalid input which would result in SQL injection results
//in an ERROR
TagIter SearchWithExpr(TodoCtx* ctx, SString data, char** errmsg);



//config
Config GetConfig(Allocator a);


#endif
