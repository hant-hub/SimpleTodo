#include <core.h>

int main() {
    logfile->buffered = FALSE;
    TodoCtx db = InitDataBase((Config){.db_loc = sstring("/tmp/database")});
    ClearDataBase(&db);
    return 0;
}

