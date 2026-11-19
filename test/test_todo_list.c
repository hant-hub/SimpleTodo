#include <core.h>
#include <ds/ds.h>

int main() {
    logfile->buffered = FALSE;
    TodoCtx ctx = InitDataBase(sstring("db_loc: /tmp/database"));
    ClearDataBase(&ctx);


    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

    u64 todo1 = AddTodo(&ctx, sstring("todo1"), sstring(""), 0, 100);
    u64 todo2 = AddTodo(&ctx, sstring("todo2"), sstring(""), 0.00001, 0);
    u64 todo3 = AddTodo(&ctx, sstring("todo3"), sstring(""), 1.0, 50);
    u64 todo4 = AddTodo(&ctx, sstring("todo4"), sstring(""), 0.001, 0);
    u64 todo5 = AddTodo(&ctx, sstring("todo5"), sstring(""), 0, 0);

    char* err;
    TagIter it = SearchWithExpr(&ctx, 0, 0, sstring("ANY"), &err);
    assert(!err);

    SString out = StringifyTodoList(&ctx, it, sc.arena);
    printlog("- [Keep] Completion%% (Priority) Name :tags\n");
    printlog("%s\n", out);


    //DeStringifyTodoList(&ctx, out, scratch, &err);

    return 0;
}
