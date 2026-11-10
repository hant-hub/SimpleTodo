#include <core.h>


int main() {
    logfile->buffered = FALSE;

    SString test_config = sstring(
        "val1: test2\n"
        "val2: test2\n"
        "val3: test3\n"
        "addkey: :a\n"
        "addkey: :b\n"
        "addkey: :c\n"
        );

    ConfigMap m = { GlobalAllocator };

    ParseConfig(&m, test_config);

    printlog("Map:\n");
    for (u32 i = 0; i < m.cap; i++) {
        if (m.keys[i].m == -1) continue;
        printlog("\t%s => [", m.keys[i].k);
        for (u32 j = 0; j < m.vals[i].vals.size; j++) {
            printlog("%s", m.vals[i].vals.data[j]);
            if (j < m.vals[i].vals.size - 1) printlog(", ");
        }
        printlog("]\n");
    }

    ConfigMapFree(&m);
}
