// rc_codegen_test.cpp — END-TO-END proof of the .rc pipeline: the smfc_rc
// tool runs at BUILD time on tests/fixtures/test_dialog.rc, its generated C++
// is compiled and linked into this program, and here we query the registry at
// run time exactly as a driver would. This is the compile-time-codegen model
// working from .rc all the way to an in-memory, queryable dialog template.
#include "dialog_ir.h"

#include <cstdio>

static int g_failures = 0;
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FAIL: %s (line %d)\n", #cond, __LINE__);        \
            ++g_failures;                                                \
        }                                                                \
    } while (0)

using namespace smfc;

int main()
{
    // The generated translation unit registered IDD_SAMPLE (1000) at static
    // init; find it the way CDialog::DoModal will.
    const DialogDesc* d = FindDialog(1000);
    CHECK(d != nullptr);
    if (!d) { std::printf("IDD_SAMPLE not registered\n"); return 1; }

    CHECK(d->idName == "IDD_SAMPLE");
    CHECK(d->controls.size() == 11);
    CHECK(AllDialogs().size() >= 1);
    CHECK(FindDialog(999999) == nullptr);

    // Spot-check a couple of controls survived codegen intact.
    bool sawList = false, sawEdit = false;
    for (const auto& c : d->controls) {
        if (c.id == 1006 && c.kind == ControlKind::Custom &&
            c.windowClass == "SysListView32")
            sawList = true;
        if (c.id == 1001 && c.kind == ControlKind::Edit)
            sawEdit = true;
    }
    CHECK(sawList);
    CHECK(sawEdit);

    if (g_failures == 0)
        std::printf("rc_codegen_test: .rc -> generated C++ -> registry query OK\n");
    return g_failures == 0 ? 0 : 1;
}
