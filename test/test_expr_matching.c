#include <core.h>

int main() {
    logfile->buffered = FALSE;
    TodoCtx ctx = InitDataBase(sstring("db_loc: /tmp/database"));
    ClearDataBase(&ctx);

    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

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

    //Add Links
    AddLink(&ctx, todo1, tag1);
    AddLink(&ctx, todo1, tag3);
    AddLink(&ctx, todo1, tag5);

    AddLink(&ctx, todo2, tag2);
    AddLink(&ctx, todo2, tag4);

    AddLink(&ctx, todo3, tag1);

    AddLink(&ctx, todo4, tag2);

    {
        bool8 found[2] = {0};
        TagIter it = SearchWithExpr(&ctx, sstring(":tag1"), NULL);
        while (1) {
            Todo t;
            IterState s = NextTodo(&it, scratch, &t);
            if (s == ITER_ERR) panic();
            if (s == ITER_DONE) break;

            if (t.id == todo1) { found[0] = TRUE; continue; }
            if (t.id == todo3) { found[1] = TRUE; continue; }
            panic();
        }
        EndTagIter(&it);

        for (u32 i = 0; i < ARRAY_SIZE(found); i++) {
            assert(found[i]);
        }
    }

    {
        bool8 found[3] = {0};
        TagIter it = SearchWithExpr(&ctx, sstring(":tag3 OR :tag2"), NULL);
        while (1) {
            Todo t;
            IterState s = NextTodo(&it, scratch, &t);
            if (s == ITER_ERR) panic();
            if (s == ITER_DONE) break;

            if (t.id == todo1) { found[0] = TRUE; continue; }
            if (t.id == todo2) { found[1] = TRUE; continue; }
            if (t.id == todo4) { found[2] = TRUE; continue; }
            panic();
        }
        EndTagIter(&it);

        for (u32 i = 0; i < ARRAY_SIZE(found); i++) {
            assert(found[i]);
        }
    }

    {
        TagIter it = SearchWithExpr(&ctx, sstring(":tag3 AND :tag2"), NULL);
        while (1) {
            Todo t;
            IterState s = NextTodo(&it, scratch, &t);
            if (s == ITER_ERR) panic();
            if (s == ITER_DONE) break;

            panic();
        }
        EndTagIter(&it);
    }

    {
        bool8 found[1] = {0};
        TagIter it = SearchWithExpr(&ctx, sstring("(:tag3 OR :tag2) AND :tag1"), NULL);
        while (1) {
            Todo t;
            IterState s = NextTodo(&it, scratch, &t);
            if (s == ITER_ERR) panic();
            if (s == ITER_DONE) break;

            if (t.id == todo1) { found[0] = TRUE; continue; }
            panic();
        }
        EndTagIter(&it);

        for (u32 i = 0; i < ARRAY_SIZE(found); i++) {
            assert(found[i]);
        }
    }

    ScratchArenaEnd(sc);
    return 0;
}
