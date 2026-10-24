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

    app = EndCompile(&g, app);
    Exec(app, 4);

    FreeTaskGraph(&g);
}
