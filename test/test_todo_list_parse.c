#include <core.h>
#include <ds/ds.h>

int main() {
    logfile->buffered = FALSE;
    TodoCtx ctx = InitDataBase(sstring("db_loc: /tmp/database"));
    ClearDataBase(&ctx);


    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

    u64 id = AddTodo(&ctx, sstring("todo4"), sstring(""), 0.0, 0);
    AddTodo(&ctx, sstring("todo3"), sstring("test todo"), 0.0, 0);

    u64 taq3 = AddTag(&ctx, sstring("tag3"));
    u64 tag4 = AddTag(&ctx, sstring("tag4"));

    AddLink(&ctx, id, taq3);
    AddLink(&ctx, id, tag4);

    SString data = sstring(
            "- [X] 50%  (1)     todo3 :tag1 :tag2\n"
            "- [X] 0%   (0.001) todo4 :tag3\n"
            " [ ] 0% (0.0) messed_up\n"
            "- [X] 0%   (0.0)   todo2\n"
            "- [ ] 0   (0.0)    messed_up\n"
            "- [X] 100% (0.0)   todo1\n"
            "- [] 100% (0.0)   todo1\n"
            "- [ ] 100% (0.0)   todo6\n"
            "- [X] 0%   (0.0)   todo5\n"
            );

    char* err;
    DeStringifyTodoList(&ctx, data, sc.arena, &err);
    if (err) {
        printlog("%n\n\n", err);
    }

    TagIter it = SearchWithExpr(&ctx, 0, 0, sstring("ANY"), &err);
    assert(!err);

    SString out = StringifyTodoList(&ctx, it, sc.arena);
    printlog("[Keep] Completion%% (Priority) Name\n");
    printlog("%s\n", out);


    Todo t = GetTodoName(&ctx, scratch, sstring("todo3"));
    printlog("Todo: %s\n", StringifyTodo(&ctx, &t, sc.arena));


    return 0;
}
