#include <core/cutils.h>
#include <sqlite3.h>
#include <core.h>

#include <stdlib.h>

#define ARGS \
    X(name, string)

#include "sa.h"

const char* helpmsg = "Usage: stedit [OPTIONS] [NAME]\n"
                "---\n"
                "Opens Markdown of Todo which can be edited.\n"
                "By default uses the editor in the EDITOR env variable.\n";

int main(int argc, char* argv[]) {
    logfile->buffered = FALSE;
    if (argc < 2) {
        printlog(helpmsg);
        return 1;
    }

    SA s = Parse(argc, argv);

    if (s.err) {
        printlog(helpmsg);
        return 1;
    }

    TodoCtx ctx = InitDataBase((Config){0});
    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

    SAvals vals = s.vals;

    SString name = (SString) {
        .data = (i8*)vals.name,
        .len = strlen(vals.name)
    };

    Todo t = GetTodoName(&ctx, scratch, name);
    bool8 new = FALSE;
    if (t.id == 0) {
        t.id = AddTodo(&ctx, name, sstring(""), 0, 0);
        new = TRUE;
    }

    SString buf = StringifyTodo(&ctx, &t, sc.arena);
    SString file_buf = {
        .data = ArenaAllocZero(sc.arena, 5 + 3 + name.len),
        .len = 5 + 3 + name.len
    };

    SString file_cmd = {
        .data = ArenaAllocZero(sc.arena, 5 + 3 + 8 + name.len + 1),
        .len = 5 + 3 + 8 + name.len
    };

    sformat(file_buf, "/tmp/%s.md", name);
    sformat(file_cmd, "$EDITOR /tmp/%s.md", name);

    filesave(file_buf, buf);
    int res = system((char*)file_cmd.data);

    if (res == 1 && new) {
        //error occurred
        DeleteTodoID(&ctx, t.id);
        return 1;
    }

    file f = fileopen(file_buf, FILE_READ);

    SString data = {
        .len = f.stats.size,
        .data = ArenaAllocZero(sc.arena, f.stats.size)
    };
    fileread(data, f);
    fileclose(f);


    printlog("Data:\n%s\n", data);

    char* err;
    Todo todo = DeStringifyTodo(&ctx, data, ArenaGetAllocator(sc.arena), &err);

    if (err) {
        printlog("Error: %n\n", err);
        return 1;
    }

    ScratchArenaEnd(sc);
    filedelete(file_buf);

    return 0;
}
