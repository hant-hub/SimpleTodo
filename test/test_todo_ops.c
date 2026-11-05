#include <core.h>

int main() {
    logfile->buffered = FALSE;
    TodoCtx ctx = InitDataBase((Config){.db_loc = sstring("/tmp/database")});
    ClearDataBase(&ctx);

    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

    {
        //test insertion actually works
        u64 todo_id = AddTodo(&ctx, sstring("test1"), sstring("todo body"), 0, TODO_NOT_STARTED);
        Todo t = GetTodoID(&ctx, scratch, todo_id);

        assert(t.id == todo_id);
        assert(Sstrcmp(t.name, sstring("test1")));
        assert(Sstrcmp(t.body, sstring("todo body")));

        //test deletion
        DeleteTodoID(&ctx, todo_id);
        t = GetTodoID(&ctx, scratch, todo_id);

        assert(t.id == todo_id);
        assert(t.name.data == NULL);
        assert(t.body.data == NULL);

        //test uniqueness
        todo_id = AddTodo(&ctx, sstring("test1"), sstring("todo body"), 0, TODO_NOT_STARTED);
        u64 todo_id2 = AddTodo(&ctx, sstring("test1"), sstring("todo body 2"), 0, TODO_NOT_STARTED);

        t = GetTodoID(&ctx, scratch, todo_id);

        assert(todo_id == todo_id2);
        assert(t.id == todo_id);
        assert(Sstrcmp(t.name, sstring("test1")));
        assert(Sstrcmp(t.body, sstring("todo body 2")));

        ClearDataBase(&ctx);
    }

    {
        //test insertion actually works
        u64 todo_id = AddTodo(&ctx, sstring("test1"), sstring("todo body"), 0, TODO_NOT_STARTED);
        Todo t = GetTodoName(&ctx, scratch, sstring("test1"));

        assert(t.id == todo_id);
        assert(Sstrcmp(t.name, sstring("test1")));
        assert(Sstrcmp(t.body, sstring("todo body")));

        //test deletion
        DeleteTodoName(&ctx, sstring("test1"));
        t = GetTodoName(&ctx, scratch, sstring("test1"));

        assert(t.id == 0);
        assert(Sstrcmp(t.name, sstring("test1")));
        assert(t.body.data == NULL);

        //test uniqueness
        todo_id = AddTodo(&ctx, sstring("test1"), sstring("todo body"), 0, TODO_NOT_STARTED);
        u64 todo_id2 = AddTodo(&ctx, sstring("test1"), sstring("todo body 2"), 0, TODO_NOT_STARTED);

        t = GetTodoName(&ctx, scratch, sstring("test1"));

        assert(todo_id == todo_id2);
        assert(t.id == todo_id);
        assert(Sstrcmp(t.name, sstring("test1")));
        assert(Sstrcmp(t.body, sstring("todo body 2")));

        ClearDataBase(&ctx);
    }

    return 0;
}
