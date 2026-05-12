// jenuroix_compiler.cpp — Jenuroix Engine build tool
// Standalone executable: no engine dependencies, no OpenGL, no ImGui.
// Just C++20 + STL + WinAPI (or POSIX).
//
// Build target: jenuroix_compiler
// Usage:
//   jenuroix_compiler              interactive menu
//   jenuroix_compiler game         build game + launch
//   jenuroix_compiler game-norun   build game, no launch
//   jenuroix_compiler editor       build Jenuroix Editor
//   jenuroix_compiler debug        debug build
//   jenuroix_compiler clean        delete build/
//   jenuroix_compiler vs           generate Visual Studio .sln
//   jenuroix_compiler all          build all targets

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <shellapi.h>
#  ifdef _MSC_VER
#    pragma comment(lib, "shell32.lib")
#  endif
#  define CLEAR_CMD  "cls"
#  define EXE_EXT    ".exe"
#  define NULL_REDIR " >nul 2>&1"
#else
#  include <unistd.h>
#  define CLEAR_CMD  "clear"
#  define EXE_EXT    ""
#  define NULL_REDIR " >/dev/null 2>&1"
#endif

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
//  Console color helpers
// ─────────────────────────────────────────────────────────────────────────────

enum Color { DEFAULT_COL = 0, YELLOW, CYAN, GREEN, RED, GRAY, WHITE };

static void setColor(Color c) {
#ifdef _WIN32
    static const WORD w[] = { 7, 14, 11, 10, 12, 8, 15 };
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), w[(int)c]);
#else
    static const char* e[] = {
        "\033[0m","\033[33m","\033[36m","\033[32m",
        "\033[31m","\033[90m","\033[97m"
    };
    fputs(e[(int)c], stdout);
#endif
}
static void reset() { setColor(DEFAULT_COL); }

static void cprint(Color c, const char* s) { setColor(c); fputs(s, stdout); reset(); }

static void tag(const char* lbl, Color c, const char* msg) {
    printf("  "); cprint(c, lbl); printf(" %s\n", msg);
}
static void ok  (const char* m) { tag("[OK]",    GREEN,  m); }
static void info(const char* m) { tag("[..]",    CYAN,   m); }
static void warn(const char* m) { tag("[WARN]",  YELLOW, m); }
static void fail(const char* m) { tag("[ERROR]", RED,    m); }
static void hr() { setColor(GRAY); puts("  ------------------------------------------"); reset(); }

static void pressEnter() {
    printf("\n  Press Enter to continue...");
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Path helpers
// ─────────────────────────────────────────────────────────────────────────────

static fs::path exeDir() {
#ifdef _WIN32
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return fs::path(buf).parent_path();
#else
    char buf[4096] = {};
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) return fs::path(std::string(buf, n)).parent_path();
    return fs::current_path();
#endif
}

// Walk up until CMakeLists.txt found = project root
static fs::path findRoot() {
    fs::path p = exeDir();
    for (int i = 0; i < 10; ++i) {
        if (fs::exists(p / "CMakeLists.txt")) return p;
        auto up = p.parent_path();
        if (up == p) break;
        p = up;
    }
    return exeDir();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tool detection
// ─────────────────────────────────────────────────────────────────────────────

static bool cmdOk(const char* cmd) {
    return system((std::string(cmd) + NULL_REDIR).c_str()) == 0;
}
static bool cmakeOk() { return cmdOk("cmake --version"); }
static bool ninjaOk() { return cmdOk("ninja --version"); }
static bool mingwOk() { return cmdOk("mingw32-make --version"); }

static std::string pythonCmd() {
    if (cmdOk("python --version"))  return "python";
    if (cmdOk("python3 --version")) return "python3";
    if (cmdOk("py --version"))      return "py";
    return {};
}

struct Generator { std::string name, extra, label; };

static Generator detectGenerator() {
#ifdef _WIN32
    struct { const char* path; Generator g; } vs[] = {
        {"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",
         {"Visual Studio 17 2022","-A x64","Visual Studio 2022"}},
        {"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe",
         {"Visual Studio 17 2022","-A x64","Visual Studio 2022"}},
        {"C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe",
         {"Visual Studio 17 2022","-A x64","Visual Studio 2022"}},
        {"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",
         {"Visual Studio 16 2019","-A x64","Visual Studio 2019"}},
        {"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe",
         {"Visual Studio 16 2019","-A x64","Visual Studio 2019"}},
        {"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe",
         {"Visual Studio 16 2019","-A x64","Visual Studio 2019"}},
    };
    for (auto& v : vs) if (fs::exists(v.path)) return v.g;
#endif
    if (ninjaOk()) return {"Ninja",         "", "Ninja"};
    if (mingwOk()) return {"MinGW Makefiles","", "MinGW"};
    return             {"Unix Makefiles",   "", "Make" };
}

// ─────────────────────────────────────────────────────────────────────────────
//  Build context
// ─────────────────────────────────────────────────────────────────────────────

struct Ctx { fs::path root, buildDir; };

// ─────────────────────────────────────────────────────────────────────────────
//  Operations
// ─────────────────────────────────────────────────────────────────────────────

static bool embedAssets(const Ctx& c) {
    auto py = c.root / "tools" / "embed_assets.py";
    if (!fs::exists(py)) { warn("tools/embed_assets.py not found — skipping"); return true; }
    auto pyCmd = pythonCmd();
    if (pyCmd.empty()) { warn("Python not found — skipping asset embed"); return true; }
    info("Embedding assets...");
    std::string run = pyCmd
        + " \"" + py.string() + "\""
        + " \"" + (c.root / "assets").string() + "\""
        + " \"" + (c.root / "src" / "EmbeddedAssets.cpp").string() + "\""
        + " \"" + (c.root / "src" / "EmbeddedAssets.h").string() + "\"";
    if (system(run.c_str()) != 0) { fail("embed_assets.py failed"); return false; }
    ok("Assets embedded");
    return true;
}

static bool doBuild(const Ctx& c, const std::string& target, const std::string& cfg) {
    printf("\n");
    if (!cmakeOk()) { fail("CMake not found — https://cmake.org/download/"); return false; }
    ok("CMake found");

    if (!fs::exists(c.root / "vendor" / "glad" / "src" / "glad.c")) {
        fail("vendor/glad/src/glad.c not found — run setup first");
        return false;
    }
    ok("GLAD found");

    if (!embedAssets(c)) return false;

    Generator gen = detectGenerator();
    ok(("Generator: " + gen.label).c_str());

    std::error_code ec;
    fs::create_directories(c.buildDir, ec);

    info("CMake configure...");
    std::string cfgCmd =
        "cmake -S \"" + c.root.string() + "\""
        " -B \""    + c.buildDir.string() + "\""
        " -G \""    + gen.name + "\" " + gen.extra +
        " -DCMAKE_BUILD_TYPE=" + cfg;
    if (system(cfgCmd.c_str()) != 0) { fail("CMake configure failed"); return false; }

    info(("Building: " + target + "  [" + cfg + "]").c_str());

    int cpus = 4;
#ifdef _WIN32
    SYSTEM_INFO si{}; GetSystemInfo(&si); cpus = (int)si.dwNumberOfProcessors;
#elif defined(_SC_NPROCESSORS_ONLN)
    cpus = (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif

    std::string bldCmd =
        "cmake --build \"" + c.buildDir.string() + "\""
        " --config " + cfg +
        " --target " + target +
        " --parallel " + std::to_string(cpus);
    if (system(bldCmd.c_str()) != 0) { fail("Build failed"); return false; }

    printf("\n");
    ok(("Done — target: " + target + "  [" + cfg + "]").c_str());
    return true;
}

static void doLaunch(const Ctx& c, const std::string& target) {
    for (auto& p : std::vector<fs::path>{
        c.buildDir / "Release" / (target + EXE_EXT),
        c.buildDir / (target + EXE_EXT),
        c.buildDir / "Debug"   / (target + EXE_EXT),
    }) {
        if (!fs::exists(p)) continue;
        info(("Launching: " + p.filename().string()).c_str());
#ifdef _WIN32
        ShellExecuteW(nullptr, L"open", p.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
        system(("\"" + p.string() + "\" &").c_str());
#endif
        return;
    }
    warn("Executable not found — check build output above");
}

static void doClean(const Ctx& c) {
    printf("\n");
    if (!fs::exists(c.buildDir)) { ok("build/ is already clean"); return; }
    info(("Removing: " + c.buildDir.string()).c_str());
    std::error_code ec;
    fs::remove_all(c.buildDir, ec);
    if (ec) { fail(("Error: " + ec.message()).c_str()); return; }
    ok("build/ deleted");
}

static void doVS(const Ctx& c) {
    printf("\n");
    if (!cmakeOk()) { fail("CMake not found"); return; }
    info("Generating Visual Studio solution...");
    std::error_code ec;
    fs::create_directories(c.buildDir, ec);

    for (auto& p : std::vector<std::pair<std::string,std::string>>{
        {"Visual Studio 17 2022", "-A x64"},
        {"Visual Studio 16 2019", "-A x64"},
    }) {
        std::string cmd =
            "cmake -S \"" + c.root.string() + "\""
            " -B \""    + c.buildDir.string() + "\""
            " -G \""    + p.first + "\" " + p.second + NULL_REDIR;
        if (system(cmd.c_str()) == 0) {
            ok(("Solution generated — " + p.first).c_str());
            for (auto& e : fs::directory_iterator(c.buildDir)) {
                if (e.path().extension() != ".sln") continue;
                info(("Opening: " + e.path().filename().string()).c_str());
#ifdef _WIN32
                ShellExecuteW(nullptr, L"open",
                    e.path().wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#endif
                return;
            }
            warn(".sln not found — open build/ manually");
            return;
        }
    }
    fail("Visual Studio 2019/2022 not found");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Interactive menu
// ─────────────────────────────────────────────────────────────────────────────

struct Item {
    std::string key;
    std::string label;
    std::function<void()> action; // nullptr = quit
};

static void drawMenu(const std::vector<Item>& items, const Ctx& c) {
    system(CLEAR_CMD);
    printf("\n");
    setColor(YELLOW);
    puts("  ==========================================");
    puts("    Jenuroix Engine  -  Compiler");
    puts("  ==========================================");
    reset();
    printf("\n");
    setColor(GRAY);
    printf("  root:   %s\n", c.root.string().c_str());
    printf("  build:  %s\n", c.buildDir.string().c_str());
    reset();
    printf("\n");

    for (auto& it : items) {
        printf("  ");
        setColor(CYAN);  printf("%s", it.key.c_str()); reset();
        printf("   %s\n", it.label.c_str());
    }
    printf("\n");
    setColor(YELLOW); printf("  > "); reset();
    fflush(stdout);
}

static void runInteractive(const Ctx& c) {
    std::vector<Item> items = {
        {"1", "Build game             (Release, launch after)", [&]{
            if (doBuild(c, "game", "Release")) doLaunch(c, "game");
            pressEnter();
        }},
        {"2", "Build game             (Release, no launch)", [&]{
            doBuild(c, "game", "Release");
            pressEnter();
        }},
        {"3", "Build Jenuroix Editor", [&]{
            doBuild(c, "editor", "Release");
            pressEnter();
        }},
        {"4", "Build debug            (Debug, no launch)", [&]{
            doBuild(c, "game", "Debug");
            pressEnter();
        }},
        {"5", "Build all targets", [&]{
            doBuild(c, "all", "Release");
            pressEnter();
        }},
        {"6", "Clean  build/          (delete everything)", [&]{
            doClean(c);
            pressEnter();
        }},
        {"7", "Prepare for Visual Studio  (generate .sln)", [&]{
            doVS(c);
            pressEnter();
        }},
        {"8", "Launch last build", [&]{
            doLaunch(c, "game");
            pressEnter();
        }},
        {"q", "Quit", nullptr},
    };

    while (true) {
        drawMenu(items, c);

        char line[32] = {};
        if (!fgets(line, sizeof(line), stdin)) break;
        for (char& ch : line) if (ch == '\n' || ch == '\r') { ch = '\0'; break; }

        if (line[0] == 'q' || line[0] == 'Q') break;

        bool found = false;
        for (auto& it : items) {
            if (it.key == line && it.action) {
                printf("\n"); hr();
                it.action();
                found = true;
                break;
            }
        }
        if (!found && line[0] != '\0') {
            printf("\n  Unknown option: %s\n", line);
            pressEnter();
        }
    }

    system(CLEAR_CMD);
    cprint(YELLOW, "  Bye.\n");
}

// ─────────────────────────────────────────────────────────────────────────────
//  CLI (non-interactive)
// ─────────────────────────────────────────────────────────────────────────────

static void runCLI(const Ctx& c, std::string cmd) {
    for (char& ch : cmd) ch = (char)tolower((unsigned char)ch);

    if      (cmd == "game")       { if (doBuild(c,"game","Release"))  doLaunch(c,"game"); }
    else if (cmd == "game-norun") { doBuild(c,"game","Release"); }
    else if (cmd == "editor")     { doBuild(c,"editor","Release"); }
    else if (cmd == "debug")      { doBuild(c,"game","Debug"); }
    else if (cmd == "all")        { doBuild(c,"all","Release"); }
    else if (cmd == "clean")      { doClean(c); }
    else if (cmd == "vs")         { doVS(c); }
    else {
        fail(("Unknown command: " + cmd).c_str());
        puts("\n  Commands:");
        puts("    game         build game + launch");
        puts("    game-norun   build game, no launch");
        puts("    editor       build Jenuroix Editor");
        puts("    debug        debug build");
        puts("    all          build all targets");
        puts("    clean        delete build/");
        puts("    vs           generate Visual Studio .sln");
    }
    printf("\n");
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    Ctx ctx{ findRoot(), {} };
    ctx.buildDir = ctx.root / "build";

    if (argc >= 2)
        runCLI(ctx, argv[1]);
    else
        runInteractive(ctx);

    return 0;
}
