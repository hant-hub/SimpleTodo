#include <core.h>

sqlite3* InitDataBase(const char* database_loc) {
    sqlite3* db;
    char* err;

    assert(!sqlite3_open(database_loc, &db));
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

    return db;
}

void DeleteTodoID(sqlite3* db, u64 t) {
    static sqlite3_stmt* statement = NULL;

    DeleteLinkTodo(db, t);
    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(db, "DELETE FROM todos WHERE rowid = ?", -1, &statement, NULL));
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

void DeleteTodoName(sqlite3* db, SString name) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(db, "DELETE FROM todos WHERE name = ?", -1, &statement, NULL));
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

Todo GetTodoID(sqlite3* db, Allocator a, u64 id) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        //we already know the id
        SQL_CALL(sqlite3_prepare_v2(db, "SELECT name, body FROM todos WHERE rowid = ?" , -1, &statement, NULL));
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

Todo GetTodoName(sqlite3* db, Allocator a, SString name) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        //we already know the id
        SQL_CALL(sqlite3_prepare_v2(db, "SELECT id, body FROM todos WHERE name = ?" , -1, &statement, NULL));
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


TodoIter BeginTodoIter(sqlite3* db, u64 todo) {
    sqlite3_stmt* statement = NULL;

    //we already know the id
    SQL_CALL(sqlite3_prepare_v2(db, 
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



void DeleteTagID(sqlite3* db, u64 id) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(db, "DELETE FROM tags WHERE rowid = ?" , -1, &statement, NULL));
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

void DeleteTagName(sqlite3* db, SString name) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(db, "DELETE FROM tags WHERE name = ?", -1, &statement, NULL));
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

Tag GetTagID(sqlite3* db, Allocator a, u64 id) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        //we already know the id
        SQL_CALL(sqlite3_prepare_v2(db, "SELECT name FROM tags WHERE rowid = ?" , -1, &statement, NULL));
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

Tag GetTagName(sqlite3* db, Allocator a, SString name) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        //we already know the id
        SQL_CALL(sqlite3_prepare_v2(db, "SELECT id FROM tags WHERE name = ?" , -1, &statement, NULL));
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

TagIter BeginTagIter(sqlite3* db, u64 tag) {
    sqlite3_stmt* statement = NULL;

    //we already know the id
    SQL_CALL(sqlite3_prepare_v2(db, 
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


void DeleteLinkID(sqlite3* db, u64 todo, u64 tag) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(db, "DELETE FROM todo_tags WHERE todo = ? AND tag = ?", -1, &statement, NULL));
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

void DeleteLinkTodo(sqlite3* db, u64 todo) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(db, "DELETE FROM todo_tags WHERE todo = ?", -1, &statement, NULL));
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

void DeleteLinkTag(sqlite3* db, u64 tag) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(db, "DELETE FROM todo_tags WHERE tag = ?", -1, &statement, NULL));
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

void AddLink(sqlite3* db, u64 todo, u64 tag) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        SQL_CALL(sqlite3_prepare_v2(db, "INSERT INTO todo_tags(todo, tag) VALUES(?, ?) ON CONFLICT(todo, tag) DO NOTHING", -1, &statement, NULL));
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

bool8 GetLink(sqlite3* db, u64 todo, u64 tag) {
    static sqlite3_stmt* statement = NULL;

    if (!statement) {
        //we already know the id
        SQL_CALL(sqlite3_prepare_v2(db, "SELECT * FROM todo_tags WHERE todo = ? AND tag = ?" , -1, &statement, NULL));
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
