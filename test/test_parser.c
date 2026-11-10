#include <core.h>
#include <ds/ds.h>

int main() {
    logfile->buffered = FALSE;

    {
        SString data = sstring(":req");

        ParsingData p = {
            .tok = {
                .data = (char*)data.data,
                .max = data.len,
            },
            .query = {
                .a = GlobalAllocator
            }
        };

        SString prefix = sstring(
                "SELECT todos.id, todos.name, todos.body, todos.priority AS PRIORITY, todos.completion AS COMPLETION "
                "FROM todos LEFT JOIN todo_tags ON todos.id = todo_tags.todo "
                "LEFT JOIN tags ON tags.id = todo_tags.tag "
                "GROUP BY todos.id "
                "HAVING ");

        dynExt(p.query, prefix.data, prefix.len);

        int r = setjmp(p.err.handler);
        if (r) {
            //handle error
            u32 len = errormsgs[p.err.code].len + 2 * token_names[p.tok.token.t].len + 2;
            char* msg = Alloc(GlobalAllocator, len);
            memset(msg, 0, len);
            sformat((SString){len, (i8*)msg}, (char*)errormsgs[p.err.code].data, token_names[p.tok.token.t], p.tok.token.token);

            printlog("%n\n", msg);

            return 1;
        }

        _NextToken(&p.tok);
        ParseExpr(&p);

        printlog("query: %s\n", (SString){p.query.size, (i8*)p.query.data});
    }


    return 0;
}
