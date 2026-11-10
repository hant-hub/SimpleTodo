#include <core.h>

int main() {
    logfile->buffered = FALSE;
    TodoCtx db = InitDataBase(sstring("db_loc: /tmp/database"));
    ClearDataBase(&db);
    return 0;
}

