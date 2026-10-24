#ifndef CORE_H
#define CORE_H

#include <core/cutils.h>
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

sqlite3* InitDataBase(const char* database_loc);

//Todo

void DeleteTodoID(sqlite3* db, u64 t);
void DeleteTodoName(sqlite3* db, SString name);

u64 AddTodo(sqlite3* db, SString name, SString body);
Todo GetTodoID(sqlite3* db, Allocator a, u64 id);
Todo GetTodoName(sqlite3* db, Allocator a, SString name);

typedef enum IterState {
    ITER_DONE,
    ITER_PENDING,
    ITER_ERR,
} IterState;

TodoIter BeginTodoIter(sqlite3* db, u64 todo);
void ResetTodoIter(TodoIter* todo);
IterState NextTag(TodoIter* iter, Allocator a, Tag* out);
void EndTodoIter(TodoIter* iter);

//Tag

void DeleteTagID(sqlite3* db, u64 id);
void DeleteTagName(sqlite3* db, SString name);

u64 AddTag(sqlite3* db, SString name);
Tag GetTagID(sqlite3* db, Allocator a, u64 id);
Tag GetTagName(sqlite3* db, Allocator a, SString name);

TagIter BeginTagIter(sqlite3* db, u64 tag);
void ResetTagIter(TagIter* todo);
IterState NextTodo(TagIter* iter, Allocator a, Todo* out);
void EndTagIter(TagIter* iter);


//Link

void DeleteLinkID(sqlite3* db, u64 todo, u64 tag);
void DeleteLinkTodo(sqlite3* db, u64 todo);
void DeleteLinkTag(sqlite3* db, u64 tag);

void AddLink(sqlite3* db, u64 todo, u64 tag);
bool8 GetLink(sqlite3* db, u64 todo, u64 tag);


#endif
