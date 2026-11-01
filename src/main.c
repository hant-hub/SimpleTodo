#include <core/cutils.h>
#include <sqlite3.h>
#include <core.h>


int main() {
    logfile->buffered = FALSE;

    bool8 initialized = filestat(sstring("./database")).stats.size;

    TodoCtx ctx = InitDataBase((Config){0});

    u64 tag1_id = AddTag(&ctx, sstring("test1"));
    u64 tag2_id = AddTag(&ctx, sstring("test2"));
    u64 tag3_id = AddTag(&ctx, sstring("test3"));
    u64 tag4_id = AddTag(&ctx, sstring("test4"));

    u64 todo1_id = AddTodo(&ctx, sstring("test1"), sstring("some random info"), 0);
    u64 todo2_id = AddTodo(&ctx, sstring("test2"), sstring("some random info"), 0);
    u64 todo3_id = AddTodo(&ctx, sstring("test3"), sstring("some random info"), 0);
    u64 todo4_id = AddTodo(&ctx, sstring("test4"), sstring("some random info"), 0);
    AddTodo(&ctx, sstring("test2"), sstring("some other random info"), 0);


    AddLink(&ctx, todo1_id, tag1_id);
    AddLink(&ctx, todo2_id, tag1_id);
    AddLink(&ctx, todo2_id, tag2_id);
    AddLink(&ctx, todo1_id, tag3_id);
    AddLink(&ctx, todo1_id, tag4_id);

    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

    Todo t = GetTodoID(&ctx, scratch, todo1_id);
    printlog("Got Todo: %s, %s\n", t.name, t.body);

    t = GetTodoName(&ctx, scratch, sstring("test2"));
    printlog("Got Todo: %s, %s\n", t.name, t.body);

    Tag tag = GetTagID(&ctx, scratch, tag1_id);
    printlog("Got Tag: %d %s\n", tag.id, tag.name);

    tag = GetTagName(&ctx, scratch, sstring("test1"));
    printlog("Got Tag: %d %s\n", tag.id, tag.name);

    bool8 linked = GetLink(&ctx, todo1_id, tag1_id);
    printlog("Todo1 <-> Tag1: %n\n", linked ? "True" : "False");

    linked = GetLink(&ctx, todo3_id, tag1_id);
    printlog("Todo3 <-> Tag1: %n\n", linked ? "True" : "False");

    //iter
    {
        TodoIter it = BeginTodoIter(&ctx, todo1_id);
        Tag t;
        printlog("Testing Todo Iter (todo1):\n");
        while (NextTag(&it, scratch, &t) != ITER_DONE) {
            printlog("\tTag: %d %s\n", t.id, t.name);
        }
        EndTodoIter(&it);
    }

    {
        TagIter it = BeginTagIter(&ctx, tag1_id);
        Todo t;
        printlog("Testing Tag Iter (tag1):\n");
        while (NextTodo(&it, scratch, &t) != ITER_DONE) {
            printlog("\tTodo: %d %s %s\n", t.id, t.name, t.body);
        }
        EndTagIter(&it);
    }

    {
        TagIter it = SearchWithExpr(&ctx, sstring("(NOT :test1 AND NOT :test2)"), NULL);
        Todo t;
        printlog("Testing Parse Iter:\n");
        while (NextTodo(&it, scratch, &t) != ITER_DONE) {
            printlog("\tTodo: %d %s %s\n", t.id, t.name, t.body);
        }
        EndTagIter(&it);
    }

    char* err;
    TagIter it = SearchWithExpr(&ctx, sstring("(NOT :test1 AND test)"), &err);
    printlog("Error: %n\n", err);

    Config c = GetConfig(scratch);
    printlog("Config:\n");
    printlog("\t db_loc: %s\n", c.db_loc);
    printlog("\t default_tags: [");
    for (u32 i = 0; i < c.default_tags.size; i++) {
        printlog(" %s", c.default_tags.data[i]);
    }
    printlog(" ]\n");

    //stringification

    Todo todo = GetTodoID(&ctx, scratch, todo1_id);
    SString str = StringifyTodo(&ctx, &todo, sc.arena);
    printlog("Str:\n%s\n", str);


    Todo todo2 = DeStringifyTodo(&ctx, str, scratch, NULL);

    assert(Sstrcmp(todo.name, todo2.name));
    assert(Sstrcmp(todo.body, todo2.body));

    Todo todo3 = DeStringifyTodo(&ctx, sstring("# a"), scratch, &err);
    printlog("Error: %n\n", err);

    ScratchArenaEnd(sc);
    return 0;
}
