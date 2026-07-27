// gui/qt/main.cpp — the process entry point the framework supplies, so an
// application does not write one. On Windows real MFC supplies WinMain and it
// does nothing but call AfxWinMain; this is that file.
//
// It holds main() and NOTHING else, so that a program with its own main (the
// tests, and anything embedding the framework) simply never pulls this object
// out of the static library. See gui/qt/afxwinmain.cpp for the other half.
#include "afxwin.h"
#include "../core/winapp_internal.h"
#include "driver_internal.h"

#include <string>
#include <vector>

int main(int argc, char** argv)
{
    // Hand the toolkit its argv before anything can need a QApplication, then
    // the command line to the framework, minus the program name - real MFC's
    // ParseCommandLine starts at the first real argument.
    smfc_qt::SetProcessArgs(argc, argv);

    std::vector<std::wstring> args;
    for (int i = 1; i < argc; ++i) {
        const std::string a(argv[i]);
        args.emplace_back(a.begin(), a.end());
    }
    smfc::SetCommandLine(args);

    return AfxWinMain(nullptr, nullptr, nullptr, 1 /*SW_SHOWNORMAL*/);
}
