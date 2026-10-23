#include <core/cutils.h>
#include <ds/ds.h>
#include <sqlite3.h>

#define MAX_TAGS 20
#define DATABASE_FILENAME "./database"

#define SQL_CALL(x) \
        do { \
            int rc = x; \
            if (rc) { \
                debugerr("SQL ERROR: %n", sqlite3_errstr(rc)); \
                panic(); \
            } \
        } while (0)

u64 AddTodo(sqlite3* db, SString name, SString body) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(db, "INSERT INTO todos(id, name, body) VALUES(NULL, ?, ?) ON CONFLICT(name) DO UPDATE SET body = excluded.body RETURNING id", -1, &statement, NULL));
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
            return id; //success
        }
        if (rc == SQLITE_BUSY) continue; //continue
        break; //error
    }

    debuglog("SQLITE ERROR: %d %n", rc, sqlite3_errstr(rc));
    panic();
}

u64 AddTag(sqlite3* db, SString name) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(db, "INSERT INTO tags(id, name, hits) VALUES(NULL, ?, 0) ON CONFLICT(name) DO UPDATE SET hits = hits + 1 RETURNING id", -1, &statement, NULL));
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

void AddLink(sqlite3* db, u64 todo, u64 tag) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(db, "INSERT INTO todo_tags(todo, tag) VALUES(?, ?)", -1, &statement, NULL));
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

int main() {
    logfile->buffered = FALSE;


    bool8 initialized = filestat(sstring(DATABASE_FILENAME)).stats.size;

    sqlite3* db;
    char* err;
    sqlite3_open(DATABASE_FILENAME, &db);
    int rc = sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, &err);

    if (rc) {
        printerr("SQLITE Error: %n\n", err);
        panic();
    }

    if (!initialized) {
        //init table schema
        int rc = sqlite3_exec(db, 
                "CREATE TABLE IF NOT EXISTS todos (id INTEGER PRIMARY KEY AUTOINCREMENT, name STRING UNIQUE, body STRING)",
                NULL, NULL, &err);

        if (rc) {
            printerr("SQLITE Error: %n\n", err);
            panic();
        }

        rc = sqlite3_exec(db, 
                "CREATE TABLE IF NOT EXISTS tags (id INTEGER PRIMARY KEY AUTOINCREMENT, name STRING UNIQUE, hits INTEGER)",
                NULL, NULL, &err);

        if (rc) {
            printerr("SQLITE Error: %n\n", err);
            panic();
        }

        rc = sqlite3_exec(db, 
                "CREATE TABLE IF NOT EXISTS todo_tags ( todo, tag, FOREIGN KEY(todo) REFERENCES todos(id), FOREIGN KEY(tag) REFERENCES tags(id) )",
                NULL, NULL, &err);

        if (rc) {
            printerr("SQLITE Error: %n\n", err);
            panic();
        }
    }

    u64 tag1_id = AddTag(db, sstring("test1"));
    u64 tag2_id = AddTag(db, sstring("test2"));

    u64 todo1_id = AddTodo(db, sstring("test1"), sstring("some random info"));
    u64 todo2_id = AddTodo(db, sstring("test2"), sstring("some random info"));
    AddTodo(db, sstring("test2"), sstring("some other random info"));

    AddLink(db, todo1_id, tag1_id);
    AddLink(db, todo2_id, tag1_id);
    AddLink(db, todo2_id, tag2_id);

    printlog("Data: (%l, %l)\n", tag1_id, todo1_id);
    printlog("Data: (%l, %l)\n", tag2_id, todo2_id);


    return 0;
}
