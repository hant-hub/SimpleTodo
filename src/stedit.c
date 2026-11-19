#include <core/cutils.h>
#include <sqlite3.h>
#include <core.h>

#include <stdlib.h>

#include <sargs/sa_pre.h>

#define PROG_NAME "stedit"
#define PROG_DESC "Tool for editing todos"

#define REQS \
    ARG(name, char*, "Todo name for editing")

#define FLAGS \
    FLAG(b, bulk, bool8, "Edit many todos at once")


#include <sargs/sa.h>

void singleEdit(SA args) {
    TodoCtx ctx = InitDataBase(sstring(""));
    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

    SString name = (SString) {
        .data = (i8*)args.args.name,
        .len = strlen(args.args.name)
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
        exit(1);
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
        exit(1);
    }

    ScratchArenaEnd(sc);
    filedelete(file_buf);
    exit(0);
}

void multiEdit(SA args) {
    TodoCtx ctx = InitDataBase(sstring(""));
    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

    if (args.flags.b.p) {}


    SString name = (SString) {
        .data = (i8*)args.args.name,
        .len = strlen(args.args.name)
    };

    char* err;
    TagIter it = SearchWithExpr(&ctx, 0, 0, name, &err);

    SString buf = StringifyTodoList(&ctx, it, sc.arena);

    SString file_buf = {
        .data = ArenaAllocZero(sc.arena, name.len + 3 + 5),
        .len = 3 + 5 + name.len
    };

    SString file_cmd = {
        .data = ArenaAllocZero(sc.arena, file_buf.len + 8),
        .len = 8 + file_buf.len 
    };

    sformat(file_buf, "/tmp/%s.md", name);
    sformat(file_cmd, "$EDITOR %s", file_buf);

    filesave(file_buf, buf);
    int res = system((char*)file_cmd.data);

    if (res == 1) {
        //error occurred
        //make no changes
        exit(1);
    }

    file f = fileopen(file_buf, FILE_READ);

    SString data = {
        .len = f.stats.size,
        .data = ArenaAllocZero(sc.arena, f.stats.size)
    };
    fileread(data, f);
    fileclose(f);

    err = NULL;
    DeStringifyTodoList(&ctx, data, sc.arena, &err);

    if (err) {
        printlog("Error: %n\n", err);
        exit(1);
    }

    ScratchArenaEnd(sc);
    filedelete(file_buf);
    exit(0);
}


int main(int argc, char* argv[]) {
    logfile->buffered = FALSE;

    SA args = ParseArgs(argc, argv);

    if (args.err) {
        printerr("%n\n", args.err);
        return 1;
    }

    if (args.flags.b.p) {
        multiEdit(args);
    } else {
        singleEdit(args);
    }

    panic();
}
