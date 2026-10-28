#define SB_IMPL
#include "./sbuild_st.h"



int main(int argc, char* argv[]) {

    AutoRebuild(argc, argv);
    setdirExe();

    TaskGraph g = CreateTaskGraph();
    //ScratchArena sc = ScratchArenaGet(NULL);

    Task* pre_compile = AddDummyTask(&g);
    {
        Task* top = NULL;
        if (!filestat(sstring("build")).stats.size) {
            Task* mkdir = AddTask(&g, "Make Build Dir");
            AddCmd(mkdir, "mkdir", "build/");
            AddCmd(mkdir, "mkdir", "build/tests");
            top = mkdir;
        }

        file f = filestat(sstring("./lib/cutils"));
        if (!f.stats.size) {
            Task* pull = AddTask(&g, "pull cutils");
            AddCmd(pull, "git", "clone", "git@eliserver:Cutils.git", "lib/cutils");

            Task* compile_build = AddTask(&g, "Make Build Script", pull);
            AddCmd(compile_build, "cc", "lib/cutils/build.c", "-o", "lib/cutils/sb");

            top = compile_build;

        }
        
        time_spec lib = filestat(sstring("build/cutils.a")).stats.modified;
        time_spec src = filestat(sstring("lib/cutils")).stats.modified;
        if (timecmp(lib, <=, src)) {
            Task* run_build = AddTask(&g, "build cutils", top);
            AddCmd(run_build, "./lib/cutils/sb", "no-test");

            Task* copy_lib = AddTask(&g, "copy cutils lib", run_build);
            AddCmd(copy_lib, "cp", "./lib/cutils/build/cutils.a", "./build");

            top = copy_lib;
        }

        AddDep(pre_compile, top);
    }

    Exec(pre_compile, 4); 
    ClearTaskGraph(&g);

    BegCompile(&g, TRUE, sstring("compile_commands.json"));
    
    //build app
    Task* app = CompileCmd(&g, "build/app", Spread("src/main.c", "src/sqlite3.c", "src/core.c", "build/cutils.a", "-Ilib/cutils/include", "-Iinclude/"));

    Task* build_tests = AddDummyTask(&g);
    ScratchArena sc = ScratchArenaGet(NULL);

    //build tests
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

        Task* t = CompileCmd(&g, (char*)output.data,
                Spread((char*)src.data, "src/sqlite3.c", "src/core.c", "build/cutils.a", "-Ilib/cutils/include", "-Iinclude/"));

        AddDep(build_tests, t);
    }

    Task* final = AddDummyTask(&g, build_tests, app);
    final = EndCompile(&g, final);
    Exec(final, 4);

    ScratchArenaEnd(sc);
    FreeTaskGraph(&g);
}
