#include <core/cutils.h>
#include <sqlite3.h>
#include <core.h>

#include <sargs/sa_pre.h>

#define PROG_NAME "stls"

#define PROG_DESC "Tool for getting tags with compound query syntax"

#define REQS \
    ARG(expr, char*, "Boolean Expression to Search DataBase")

#define FLAGS \
    FLAG(c, color, bool8, "Colorize Output Based on Completion") \
    FLAG(u, unicode, bool8, "Use Unicode in Output") \
    FLAG(p, pretty_print, bool8, "Colorize Output and Use Unicode") \
    FLAG(m, minimal, bool8, "Disable Colorization and Unicode") \
    FLAG(com, completion, bool8, "Display Completion Information") \
    FLAG(pr, priority, bool8, "Display Priority") \
    FLAG(md, markdown, bool8, "Display Output as Markdown List, --color is ignored if this is set") \
    FLAG(tag, search_tag, bool8, "Search Tags Instead of Todos, disables the ':tag' syntax") \
    FLAG(l, limit, int, "Limit the number of items shown") \
    FLAG(o, offset, int, "Ignore the first offset items. If this is set and limit is not, limit is set to 1000.")

#include <sargs/sa.h>

typedef struct PrintConfig {
    bool8 color;
    bool8 unicode;
    bool8 completion;
    bool8 priority;
    bool8 markdown;
} PrintConfig;

void PrintTodo(PrintConfig c, Todo t);
void PrintTag(PrintConfig c, Tag t);

int main(int argc, char* argv[]) {
    logfile->buffered = FALSE;
    SA s = ParseArgs(argc, argv);

    if (s.err) {
        if (strcmp(s.err, "User Displayed Help Info") == 0) {
            return 0;
        }
        return 1;
    }

    TodoCtx ctx = InitDataBase(sstring(""));

    PrintConfig config = {};

    if (s.flags.p.p) {
        config.color = TRUE;
        config.completion = TRUE;
        config.unicode = TRUE;
    }

    if (s.flags.m.p) {
        config = (PrintConfig){0};
    }

    if (s.flags.c.p) config.color = TRUE;
    if (s.flags.com.p) config.completion = TRUE;
    if (s.flags.pr.p) config.priority = TRUE;
    if (s.flags.u.p) config.unicode = TRUE;
    if (s.flags.md.p) config.markdown = TRUE;
    
    if (s.flags.tag.p) goto tag_search;

    {
        char* err;
        TagIter it = SearchWithExpr(&ctx, s.flags.l.v, s.flags.o.v, (SString){.data = (i8*)s.args.expr, .len = strlen(s.args.expr)}, &err);

        if (!it.query) {
            printlog("Error: %n\n", err);
            return 1;
        }

        if (config.markdown) {
            printlog("# TODOS:\n");
        }

        while (1) {
            ScratchArena sc = ScratchArenaGet(NULL);
            Todo t;
            IterState state = NextTodo(&it, ArenaGetAllocator(sc.arena), &t);
            if (state == ITER_DONE) break;
            if (state == ITER_ERR) break;

            PrintTodo(config, t);
            ScratchArenaEnd(sc); 
        }

        return 0;
    }

tag_search:
    {
        char* err;
        TodoIter it = SearchTags(&ctx, s.flags.l.v, s.flags.o.v, (SString){.data = (i8*)s.args.expr, .len = strlen(s.args.expr)}, &err);

        if (!it.query) {
            printlog("Error: %n\n", err);
            return 1;
        }

        if (config.markdown) {
            printlog("# Tags:\n");
        }

        while (1) {
            ScratchArena sc = ScratchArenaGet(NULL);
            Tag t;
            IterState state = NextTag(&it, ArenaGetAllocator(sc.arena), &t);
            if (state == ITER_DONE) break;
            if (state == ITER_ERR) break;

            PrintTag(config, t);
            ScratchArenaEnd(sc); 
        }


        return 0;
    }
}

void PrintTag(PrintConfig c, Tag t) {
    if (c.markdown) {
        printlog("- ");
    } 

    //always print name
    printlog("%s", t.name);
    printlog("\n");
}

void PrintTodo(PrintConfig c, Todo t) {
    if (c.markdown) {
        printlog("- ");

        if (c.completion) {
            printlog("["); 
            if (t.completion == 100) {
                printlog("X");
            } else {
                printlog(" ");
            }
            printlog("] "); 
        }

    } else {

        if (c.color) {
            if (t.completion == 100) printlog("\033[38;2;0;255;0m");
        }

        if (c.completion) {
            printlog("%d%% ", t.completion);

            if (t.completion < 10) {
                printlog("  ");
            } else if (t.completion < 100) {
                printlog(" ");
            }

            if (c.unicode) {
                if (t.completion == 100) printlog("✅ ");
                else printlog("  ");
            }
        }
    }

    //always print name
    printlog("%s", t.name);

    if (c.priority) {
        printlog(" (%f)", t.priority); 
    }

    if (!c.markdown && c.color) {
        //clear color
        printlog("\033[0m");
    }

    printlog("\n");
}
