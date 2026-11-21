#define SB_IMPL
#include "./sbuild_st.h"

int main(int argc, char* argv[]) {
    logfile->buffered = FALSE;

    AutoRebuild(argc, argv);
    setdirExe();

    TaskGraph g = CreateTaskGraph();
    ////ScratchArena sc = ScratchArenaGet(NULL);

    Task* pre_compile = AddDummyTask(&g);
    {
        Task* top = NULL;
        if (!filestat(sstring("build")).stats.size) {
            Task* mkdir = AddTask(&g, "Make Build Dir");
            //AddCmd(mkdir, "mkdir", "build/");
            AddCmd(mkdir, "mkdir", "-p", "./build/tests");
            top = mkdir;
        }

        AddDep(pre_compile, top);
    }

    BegCompile(&g, TRUE, sstring("compile_commands.json"));

    //this is dumb as hell, but for some reason
    //when a process exits it doesn't sync with the file system,
    //so sqlite3 isn't fully available when ar starts
    Task* pre_lib = AddDummyTask(&g);
    {
        Task* sqlite = CompileCmd(&g, "build/sqlite3.o", Spread("-c", "src/sqlite3.c", "-Iinclude/"), pre_compile);
        Task* core = CompileCmd(&g, "build/core.o", Spread("-c", "src/core.c", "-Iinclude/"), pre_compile);
        AddDep(pre_lib, sqlite);
        AddDep(pre_lib, core);
    }
    {
        Task* sqlite = CompileCmd(&g, "build/sqlite3_debug.o", Spread("-DDEBUG", "-g", "-c", "src/sqlite3.c", "-Iinclude/"), pre_compile);
        Task* core = CompileCmd(&g, "build/core_debug.o", Spread("-DDEBUG", "-g", "-c", "src/core.c", "-Iinclude/", "-Wall"), pre_compile);
        AddDep(pre_lib, sqlite);
        AddDep(pre_lib, core);
    }

    //build static lib
    Task* debug_lib = AddTask(&g, "build debug lib", pre_lib);
    AddCmd(debug_lib, "ar", "rcUu", "build/st_debug.a", "build/sqlite3_debug.o", "build/core_debug.o");
    AddDep(debug_lib, pre_lib);

    Task* lib = AddTask(&g, "build lib", pre_lib);
    AddCmd(lib, "ar", "rcUu", "build/st.a", "build/sqlite3.o", "build/core.o");
    AddDep(lib, pre_lib);

    
    //build app
    Task* app = CompileCmd(&g, "build/app", Spread("-g", "src/main.c", "build/st_debug.a", "-lcutils",
                "-Iinclude/"), debug_lib);

    Task* build_tests = AddDummyTask(&g);
    ScratchArena sc = ScratchArenaGet(NULL);

    //build stls
    Task* stls = CompileCmd(&g, "build/stls", Spread("src/stls.c", "build/st.a", "-lcutils",
                "-Iinclude/", "-Wall"), lib);

    Task* stedit = CompileCmd(&g, "build/stedit", Spread("src/stedit.c", "build/st.a", "-lcutils",
                "-Iinclude/", "-Wall"), lib);



    //build tests
    

    file c_file = filestat(sstring("src/core.c"));

    const SString test_dir = sstring("./test");
    DirBegin(test_dir);
    while (1) {
        DirType type;
        SString filename = DirNext(&type);
        if (type == T_DIR) continue;
        if (type == T_END) break;

        const SString prefix = sstring("./build/tests/");
        SString base = filepathbasename(filename);

        SString src = {
            .data = ArenaAllocZero(sc.arena, filename.len + test_dir.len + 2),
            .len = filename.len + test_dir.len + 2
        };
        
        SString output = {
            .data = ArenaAllocZero(sc.arena, base.len + prefix.len + 1),
            .len = base.len + prefix.len + 1
        };

        sformat(src, "%s/%s", test_dir, filename);
        sformat(output, "%s%s", prefix, base);
        printlog("Output: %n %n\n", output.data, src.data);

        file s_file = filestat(src);
        file o_file = filestat(output);

        if (timecmp(s_file.stats.modified, >=, o_file.stats.modified) ||
            timecmp(c_file.stats.modified, >=, o_file.stats.modified)) {
            Task* t = CompileCmd(&g, (char*)output.data,
                    Spread((char*)src.data, "-g", "-DDEBUG", "build/st_debug.a", "-lcutils", "-Iinclude/", "-Wall"),
                    debug_lib);
            AddDep(build_tests, t);
        }
    }

    Task* final = AddDummyTask(&g, build_tests, app, stls, stedit);
    final = EndCompile(&g, final);
    Exec(final, 2);

    ScratchArenaEnd(sc);
    FreeTaskGraph(&g);
}
