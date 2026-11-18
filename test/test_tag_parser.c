#include <core.h>
#include <ds/ds.h>

int main() {
    logfile->buffered = FALSE;

    {
        SString data = sstring("test");

        ParsingData p = {
            .tok = {
                .data = (char*)data.data,
                .max = data.len,
            },
            .flags = {
                .enable_tag = FALSE,
                .name_field = sstring("tags.name"),
            },
            .query = {
                .a = GlobalAllocator
            }
        };

        SString prefix = sstring(
                "SELECT tags.id, tags.name FROM tags "
                "GROUP BY tags.id "
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
