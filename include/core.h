#ifndef CORE_H
#define CORE_H

#include <core/cutils.h>
#include <ds/ds.h>
#include <sqlite3.h>

#define SQL_CALL(x) \
        do { \
            int rc = x; \
            if (rc) { \
                debugerr("SQL ERROR: %n", sqlite3_errstr(rc)); \
                panic(); \
            } \
        } while (0)

//IMPORTANT(ELI): name and body must be freed
typedef struct Todo {
    u64 id;
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

TodoCtx InitDataBase();
void ClearDataBase(TodoCtx* ctx);

//Todo

void DeleteTodoID(TodoCtx* ctx, u64 t);
void DeleteTodoName(TodoCtx* ctx, SString name);

u64 AddTodo(TodoCtx* ctx, SString name, SString body);
Todo GetTodoID(TodoCtx* ctx, Allocator a, u64 id);
Todo GetTodoName(TodoCtx* ctx, Allocator a, SString name);

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
TagIter SearchWithExpr(TodoCtx* ctx, SString data, char** errmsg);


Config GetConfig(Allocator a);


#endif
