#include <core.h>
#include <ds/ds.h>

int main() {
    logfile->buffered = FALSE;
    TodoCtx ctx = InitDataBase(sstring("db_loc: /tmp/database"));
    ClearDataBase(&ctx);


    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

    SString data = sstring(
            "- [X] 50%  (1)     todo3\n"
            "- [ ] 0%   (0.001) todo4\n"
            "- [X] 0%   (0.0)   todo2\n"
            "- [X] 100% (0.0)   todo1\n"
            "- [X] 0%   (0.0)   todo5\n"
            );

    char* err;
    DeStringifyTodoList(&ctx, data, scratch, &err);


    TagIter it = SearchWithExpr(&ctx, 0, 0, sstring("ANY"), &err);
    assert(!err);

    SString out = StringifyTodoList(it, sc.arena);
    printlog("[Keep] Completion%% (Priority) Name\n");
    printlog("%s\n", out);

    return 0;
}
