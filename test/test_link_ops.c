#include <core.h>
#include <string.h>

int main() {
    logfile->buffered = FALSE;
    TodoCtx ctx = InitDataBase((Config){.db_loc = sstring("/tmp/database")});
    ClearDataBase(&ctx);

    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

    //set up todos and tags
    u64 todo1 = AddTodo(&ctx, sstring("todo1"), sstring(""), 0, 0);
    u64 todo2 = AddTodo(&ctx, sstring("todo2"), sstring(""), 0, 0);
    u64 todo3 = AddTodo(&ctx, sstring("todo3"), sstring(""), 0, 0);
    u64 todo4 = AddTodo(&ctx, sstring("todo4"), sstring(""), 0, 0);
    u64 todo5 = AddTodo(&ctx, sstring("todo5"), sstring(""), 0, 0);

    u64 tag1 = AddTag(&ctx, sstring("tag1"));
    u64 tag2 = AddTag(&ctx, sstring("tag2"));
    u64 tag3 = AddTag(&ctx, sstring("tag3"));
    u64 tag4 = AddTag(&ctx, sstring("tag4"));
    u64 tag5 = AddTag(&ctx, sstring("tag5"));


    //insertion
    AddLink(&ctx, todo1, tag1);
    bool8 test = GetLink(&ctx, todo1, tag1);
    assert(test);

    //deletion
    DeleteLinkID(&ctx, todo1, tag1);
    test = GetLink(&ctx, todo1, tag1);
    assert(!test);

    //iteration todo
    AddLink(&ctx, todo1, tag1);
    AddLink(&ctx, todo1, tag2);
    AddLink(&ctx, todo1, tag3);
    AddLink(&ctx, todo1, tag4);
    AddLink(&ctx, todo1, tag5);

    bool8 found[5] = {0};
    TodoIter todo_tags = BeginTodoIter(&ctx, todo1);
    while (1) {
        Tag t;
        IterState s = NextTag(&todo_tags, scratch, &t);
        if (s == ITER_ERR) panic();
        if (s == ITER_DONE) break;

        if (t.id == tag1) found[0] = TRUE;
        if (t.id == tag2) found[1] = TRUE;
        if (t.id == tag3) found[2] = TRUE;
        if (t.id == tag4) found[3] = TRUE;
        if (t.id == tag5) found[4] = TRUE;
    }
    EndTodoIter(&todo_tags);


    for (u32 i = 0; i < 5; i++) {
        assert(found[i]);
    }

    DeleteLinkID(&ctx, todo1, tag1);
    DeleteLinkID(&ctx, todo1, tag2);
    DeleteLinkID(&ctx, todo1, tag3);
    DeleteLinkID(&ctx, todo1, tag4);
    DeleteLinkID(&ctx, todo1, tag5);


    //iteration tag

    AddLink(&ctx, todo1, tag1);
    AddLink(&ctx, todo2, tag1);
    AddLink(&ctx, todo3, tag1);
    AddLink(&ctx, todo4, tag1);
    AddLink(&ctx, todo5, tag1);

    memset(found, 0, sizeof(found));
    TagIter tag_todos = BeginTagIter(&ctx, tag1);
    while (1) {
        Todo t;
        IterState s = NextTodo(&tag_todos, scratch, &t);
        if (s == ITER_ERR) panic();
        if (s == ITER_DONE) break;

        if (t.id == todo1) found[0] = TRUE;
        if (t.id == todo2) found[1] = TRUE;
        if (t.id == todo3) found[2] = TRUE;
        if (t.id == todo4) found[3] = TRUE;
        if (t.id == todo5) found[4] = TRUE;
    }
    EndTagIter(&tag_todos);

    for (u32 i = 0; i < 5; i++) {
        assert(found[i]);
    }

    ScratchArenaEnd(sc);
    return 0;
}
