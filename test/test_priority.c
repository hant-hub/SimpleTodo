#include <core.h>
#include <string.h>

int main() {
    logfile->buffered = FALSE;
    TodoCtx ctx = InitDataBase((Config){.db_loc = sstring("/tmp/database")});
    ClearDataBase(&ctx);

    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

    //set up todos and tags
    u64 todo1 = AddTodo(&ctx, sstring("todo1"), sstring(""), 5, TODO_NOT_STARTED);
    u64 todo2 = AddTodo(&ctx, sstring("todo2"), sstring(""), 4, TODO_NOT_STARTED);
    u64 todo3 = AddTodo(&ctx, sstring("todo3"), sstring(""), 3, TODO_NOT_STARTED);
    u64 todo4 = AddTodo(&ctx, sstring("todo4"), sstring(""), 2, TODO_NOT_STARTED);
    u64 todo5 = AddTodo(&ctx, sstring("todo5"), sstring(""), 1, TODO_NOT_STARTED);

    u64 tag1 = AddTag(&ctx, sstring("tag1"));

    //insertion
    AddLink(&ctx, todo1, tag1);
    bool8 test = GetLink(&ctx, todo1, tag1);
    assert(test);

    //deletion
    DeleteLinkID(&ctx, todo1, tag1);
    test = GetLink(&ctx, todo1, tag1);
    assert(!test);

    //iteration tag
    AddLink(&ctx, todo1, tag1);
    AddLink(&ctx, todo2, tag1);
    AddLink(&ctx, todo3, tag1);
    AddLink(&ctx, todo4, tag1);
    AddLink(&ctx, todo5, tag1);

    u64 ids[] = {
        todo1,
        todo2,
        todo3,
        todo4,
        todo5,
    };
    u32 i = 0;

    TagIter tag_todos = BeginTagIter(&ctx, tag1);
    while (1) {
        Todo t;
        IterState s = NextTodo(&tag_todos, scratch, &t);
        if (s == ITER_ERR) panic();
        if (s == ITER_DONE) break;

        assert(ids[i++] == t.id);
    }
    EndTagIter(&tag_todos);

    ScratchArenaEnd(sc);
    return 0;
}
