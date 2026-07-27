// gui/core/rc/smfc_rc_main.cpp — command-line front end for the .rc compiler.
//
// Usage:
//   smfc_rc --rc app.rc [--header resource.h ...] --out generated_dialogs.cpp
//
// Invoked by CMake at build time (add_custom_command): reads the app's .rc and
// any resource.h symbol headers, and writes a generated C++ translation unit
// that registers the neutral dialog IR. This is simple_mfc's own mini rc.exe.
#include "rc_compiler.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool readFile(const std::string& path, std::string& out)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    std::string rcPath, outPath;
    std::vector<std::string> headerPaths;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](const char* what) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "smfc_rc: %s requires an argument\n", what);
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--rc") rcPath = need("--rc");
        else if (a == "--out") outPath = need("--out");
        else if (a == "--header") headerPaths.push_back(need("--header"));
        else {
            std::fprintf(stderr, "smfc_rc: unknown argument '%s'\n", a.c_str());
            return 2;
        }
    }

    if (rcPath.empty() || outPath.empty()) {
        std::fprintf(stderr,
            "usage: smfc_rc --rc FILE [--header FILE ...] --out FILE\n");
        return 2;
    }

    smfc::rc::SymbolTable symbols;
    for (const auto& h : headerPaths) {
        std::string text;
        if (!readFile(h, text)) {
            std::fprintf(stderr, "smfc_rc: cannot read header '%s'\n", h.c_str());
            return 1;
        }
        smfc::rc::SymbolTable one = smfc::rc::ParseSymbolHeader(text);
        symbols.insert(one.begin(), one.end());
    }

    std::string rcText;
    if (!readFile(rcPath, rcText)) {
        std::fprintf(stderr, "smfc_rc: cannot read .rc '%s'\n", rcPath.c_str());
        return 1;
    }

    smfc::rc::ParseResult res = smfc::rc::ParseResourceScript(rcText, symbols);
    if (!res.ok()) {
        std::fprintf(stderr, "smfc_rc: parse error: %s\n", res.error.c_str());
        return 1;
    }
    for (const auto& w : res.warnings)
        std::fprintf(stderr, "smfc_rc: warning: %s\n", w.c_str());

    std::string cpp = smfc::rc::EmitGeneratedCpp(res.dialogs, rcPath);

    std::ofstream out(outPath, std::ios::binary);
    if (!out) {
        std::fprintf(stderr, "smfc_rc: cannot write '%s'\n", outPath.c_str());
        return 1;
    }
    out << cpp;
    std::fprintf(stderr, "smfc_rc: %zu dialog(s) -> %s\n",
                 res.dialogs.size(), outPath.c_str());
    return 0;
}
