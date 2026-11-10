#include <core.h>

int main() {
    logfile->buffered = FALSE;
    TodoCtx db = InitDataBase(sstring("db_loc: /tmp/mkdr/test/database"));
    ClearDataBase(&db);
    return 0;
}
