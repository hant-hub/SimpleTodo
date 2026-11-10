#include <core/cutils.h>
#include <sqlite3.h>
#include <core.h>


#define ARGS \
    X(expr, string)

#include "sa.h"

const char* helpmsg = "Usage: stls [OPTIONS] [SEARCH EXPRESSION]\n"
                "---\n"
                "[SEARCH EXPRESSION] -> Ex: :tag1 OR :tag2\n";

int main(int argc, char* argv[]) {
    logfile->buffered = FALSE;
    if (argc < 2) {
        printlog(helpmsg);
        return 1;
    }

    SA s = Parse(argc, argv);

    if (s.err) {
        printlog(helpmsg);
        return 1;
    }

    TodoCtx ctx = InitDataBase(sstring(""));
    SAvals vals = s.vals;

    char* err;
    TagIter it = SearchWithExpr(&ctx, (SString){.data = (i8*)vals.expr, .len = strlen(vals.expr)}, &err);

    if (!it.query) {
        printlog("Error: %n\n", err);
        return 1;
    }

    while (1) {
        ScratchArena sc = ScratchArenaGet(NULL);
        Todo t;
        IterState state = NextTodo(&it, ArenaGetAllocator(sc.arena), &t);
        if (state == ITER_DONE) break;
        if (state == ITER_ERR) break;
        printlog("%s\n", t.name);
        ScratchArenaEnd(sc); 
    }
    return 0;
}
