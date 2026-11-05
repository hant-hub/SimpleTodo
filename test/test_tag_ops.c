#include <core.h>

int main() {
    logfile->buffered = FALSE;
    TodoCtx ctx = InitDataBase((Config){.db_loc = sstring("/tmp/database")});
    ClearDataBase(&ctx);
    ScratchArena sc = ScratchArenaGet(NULL);
    Allocator scratch = ArenaGetAllocator(sc.arena);

    {
        //insertion
        u64 tag_id = AddTag(&ctx, sstring("test1"));
        Tag t = GetTagID(&ctx, scratch, tag_id);

        assert(t.id == tag_id);
        assert(Sstrcmp(t.name, sstring("test1")));

        //deletion    
        DeleteTagID(&ctx, tag_id);
        t = GetTagID(&ctx, scratch, tag_id);

        assert(t.id == tag_id);
        assert(t.name.data == NULL);

        //uniqueness
        tag_id = AddTag(&ctx, sstring("test1"));
        u64 tag_id2 = AddTag(&ctx, sstring("test1"));
        t = GetTagID(&ctx, scratch, tag_id);

        assert(tag_id == tag_id2);
        assert(t.id == tag_id);
        assert(Sstrcmp(t.name, sstring("test1")));

        ClearDataBase(&ctx);
    }

    {
        //insertion
        u64 tag_id = AddTag(&ctx, sstring("test1"));
        Tag t = GetTagName(&ctx, scratch, sstring("test1"));

        assert(t.id == tag_id);
        assert(Sstrcmp(t.name, sstring("test1")));

        //deletion    
        DeleteTagName(&ctx, sstring("test1"));
        t = GetTagName(&ctx, scratch, sstring("test1"));

        assert(t.id == 0);
        assert(Sstrcmp(t.name, sstring("test1")));

        //uniqueness
        tag_id = AddTag(&ctx, sstring("test1"));
        u64 tag_id2 = AddTag(&ctx, sstring("test1"));
        t = GetTagName(&ctx, scratch, sstring("test1"));

        assert(tag_id == tag_id2);
        assert(t.id == tag_id);
        assert(Sstrcmp(t.name, sstring("test1")));

        ClearDataBase(&ctx);
    }




    ScratchArenaEnd(sc);
    return 0;
}
