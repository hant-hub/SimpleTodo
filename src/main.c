#include <core/cutils.h>
#include <sqlite3.h>
#include <core.h>


int main() {
    logfile->buffered = FALSE;

    bool8 initialized = filestat(sstring("./database")).stats.size;

    sqlite3* db = InitDataBase("./database");

    u64 tag1_id = AddTag(db, sstring("test1"));
    u64 tag2_id = AddTag(db, sstring("test2"));
    u64 tag3_id = AddTag(db, sstring("test3"));
    u64 tag4_id = AddTag(db, sstring("test4"));

    u64 todo1_id = AddTodo(db, sstring("test1"), sstring("some random info"));
    u64 todo2_id = AddTodo(db, sstring("test2"), sstring("some random info"));
    u64 todo3_id = AddTodo(db, sstring("test3"), sstring("some random info"));
    u64 todo4_id = AddTodo(db, sstring("test4"), sstring("some random info"));
    AddTodo(db, sstring("test2"), sstring("some other random info"));


    AddLink(db, todo1_id, tag1_id);
    AddLink(db, todo2_id, tag1_id);
    AddLink(db, todo2_id, tag2_id);
    AddLink(db, todo1_id, tag3_id);
    AddLink(db, todo1_id, tag4_id);

    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

    Todo t = GetTodoID(db, scratch, todo1_id);
    printlog("Got Todo: %s, %s\n", t.name, t.body);

    t = GetTodoName(db, scratch, sstring("test2"));
    printlog("Got Todo: %s, %s\n", t.name, t.body);

    Tag tag = GetTagID(db, scratch, tag1_id);
    printlog("Got Tag: %d %s\n", tag.id, tag.name);

    tag = GetTagName(db, scratch, sstring("test1"));
    printlog("Got Tag: %d %s\n", tag.id, tag.name);

    bool8 linked = GetLink(db, todo1_id, tag1_id);
    printlog("Todo1 <-> Tag1: %n\n", linked ? "True" : "False");

    linked = GetLink(db, todo3_id, tag1_id);
    printlog("Todo3 <-> Tag1: %n\n", linked ? "True" : "False");

    //iter
    {
        TodoIter it = BeginTodoIter(db, todo1_id);
        Tag t;
        printlog("Testing Todo Iter (todo1):\n");
        while (NextTag(&it, scratch, &t) != ITER_DONE) {
            printlog("\tTag: %d %s\n", t.id, t.name);
        }
        EndTodoIter(&it);
    }

    {
        TagIter it = BeginTagIter(db, tag1_id);
        Todo t;
        printlog("Testing Tag Iter (tag1):\n");
        while (NextTodo(&it, scratch, &t) != ITER_DONE) {
            printlog("\tTodo: %d %s %s\n", t.id, t.name, t.body);
        }
        EndTagIter(&it);
    }


    //deletions
    DeleteLinkID(db, todo1_id, tag1_id);
    DeleteLinkTodo(db, todo1_id);
    DeleteLinkTag(db, todo1_id);

    DeleteTodoID(db, todo3_id);
    DeleteTodoName(db, sstring("test4"));

    DeleteTagID(db, tag3_id);
    DeleteTagName(db, sstring("test4"));

    printlog("Data: (%l, %l)\n", tag1_id, todo1_id);
    printlog("Data: (%l, %l)\n", tag2_id, todo2_id);

    ScratchArenaEnd(sc);
    return 0;
}
