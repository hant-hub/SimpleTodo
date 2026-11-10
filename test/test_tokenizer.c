#include <core.h>

int main() {
    logfile->buffered = FALSE;

    {
        SString data = sstring(":tag1 name1 < > = >= <= != AND OR ANY NOT ( )");
        TokenType seq[] = {
            TOKEN_TAG, TOKEN_ID,
            TOKEN_LT, TOKEN_GT, TOKEN_EQ,
            TOKEN_GE, TOKEN_LE, TOKEN_NEQ,
            TOKEN_AND, TOKEN_OR,
            TOKEN_ANY, TOKEN_NOT, TOKEN_OPAREN, TOKEN_CPAREN,
            TOKEN_END
        };
        ExprTokenizer tok = {
            .data = (char*)data.data,
            .max = data.len
        };

        //printlog("tok: ");
        u32 curr = 0;
        while (1) {
            _NextToken(&tok);

            assert(curr < (ARRAY_SIZE(seq)));
            if (tok.token.t != seq[curr]) {
                printlog("tok: %s, Expected: %s\n", token_names[tok.token.t], token_names[seq[curr]]);

                assert(0);
            }
            curr++;
            //printlog("%s ", token_names[tok.token.t]);
            if (tok.token.t == TOKEN_END) break;
        }
        //printlog("\n");

    }


    return 0;
}
