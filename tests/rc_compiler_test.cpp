// rc_compiler_test.cpp — unit test for simple_mfc's .rc resource compiler.
// Exercises the symbol-header parser, the DIALOGEX parser and the C++ emitter
// against tests/fixtures/test_dialog.rc + test_resource.h.
#include "rc_compiler.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static int g_failures = 0;
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FAIL: %s (line %d)\n", #cond, __LINE__);        \
            ++g_failures;                                                \
        }                                                                \
    } while (0)

#ifndef SMFC_FIXTURES_DIR
#define SMFC_FIXTURES_DIR "."
#endif

static std::string slurp(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

using namespace smfc;
using namespace smfc::rc;

static const ControlDesc* find(const DialogDesc& d, int id)
{
    for (const auto& c : d.controls)
        if (c.id == id) return &c;
    return nullptr;
}

int main()
{
    const std::string dir = SMFC_FIXTURES_DIR;

    // --- symbol header --------------------------------------------------
    SymbolTable syms = ParseSymbolHeader(slurp(dir + "/test_resource.h"));
    CHECK(syms["IDD_SAMPLE"] == 1000);
    CHECK(syms["IDC_NAME_EDIT"] == 1001);
    CHECK(syms["IDC_ENABLE_CHECK"] == 1003);   // hex 0x03EB
    CHECK(syms["IDC_OPTION_A"] == 1004);
    CHECK(syms["IDC_OPTION_B"] == 1005);        // (IDC_OPTION_A + 1)
    CHECK(syms["IDC_STATIC"] == -1);

    // --- dialog parse ---------------------------------------------------
    ParseResult res = ParseResourceScript(slurp(dir + "/test_dialog.rc"), syms);
    CHECK(res.ok());
    CHECK(res.dialogs.size() == 4);   // IDD_SAMPLE, IDD_NOTTEST, IDD_AFTER, IDD_CONTROLS
    if (res.dialogs.empty()) { std::printf("no dialog parsed\n"); return 1; }

    const DialogDesc& d = res.dialogs[0];
    CHECK(d.id == 1000);
    CHECK(d.idName == "IDD_SAMPLE");
    CHECK(d.caption == "Sample \"Quoted\" Dialog");   // "" -> " decoded
    CHECK(d.cx == 220 && d.cy == 160);
    CHECK(d.fontSize == 8);
    CHECK(d.fontFace == "MS Shell Dlg");
    CHECK((d.style & 0x80u) != 0);   // DS_MODALFRAME resolved
    CHECK((d.style & 0x00C00000u) != 0); // WS_CAPTION resolved
    CHECK(d.controls.size() == 11);

    // LTEXT: static, id resolved to -1 (IDC_STATIC), text + rect
    const ControlDesc* lbl = nullptr;
    for (const auto& c : d.controls)
        if (c.kind == ControlKind::Static && c.text == "Name:") lbl = &c;
    CHECK(lbl != nullptr);
    if (lbl) { CHECK(lbl->id == -1); CHECK(lbl->x == 7 && lbl->cy == 8); }

    // EDITTEXT: no text, ES_AUTOHSCROLL resolved into style
    const ControlDesc* edit = find(d, 1001);
    CHECK(edit && edit->kind == ControlKind::Edit);
    if (edit) {
        CHECK(edit->text.empty());
        CHECK((edit->style & 0x80u) != 0);   // ES_AUTOHSCROLL
        CHECK(edit->cx == 163 && edit->cy == 14);
    }

    // DEFPUSHBUTTON
    const ControlDesc* go = find(d, 1002);
    CHECK(go && go->kind == ControlKind::DefButton && go->text == "Go");

    // Cancel button: IDCANCEL isn't in resource.h but is an SDK-standard id
    // the compiler seeds itself, so it must resolve to 2.
    const ControlDesc* cancel = find(d, 2);
    CHECK(cancel && cancel->kind == ControlKind::Button && cancel->text == "Cancel");
    if (cancel) CHECK(cancel->idName == "IDCANCEL");

    // Generic CONTROL "Button" + BS_AUTOCHECKBOX -> CheckBox
    const ControlDesc* chk = find(d, 1003);
    CHECK(chk && chk->kind == ControlKind::CheckBox);
    if (chk) CHECK(chk->text == "Enable feature");

    // GROUPBOX
    const ControlDesc* grp = find(d, 1009);
    CHECK(grp && grp->kind == ControlKind::GroupBox);

    // Radio buttons via generic CONTROL "Button" + BS_AUTORADIOBUTTON
    const ControlDesc* ra = find(d, 1004);
    const ControlDesc* rb = find(d, 1005);
    CHECK(ra && ra->kind == ControlKind::RadioButton);
    CHECK(rb && rb->kind == ControlKind::RadioButton);

    // SysListView32 -> Custom, class string preserved
    const ControlDesc* lst = find(d, 1006);
    CHECK(lst && lst->kind == ControlKind::Custom);
    if (lst) {
        CHECK(lst->windowClass == "SysListView32");
        CHECK((lst->style & 0x0001u) != 0);   // LVS_REPORT
    }

    // COMBOBOX (no text), CBS_DROPDOWNLIST resolved
    const ControlDesc* cmb = find(d, 1007);
    CHECK(cmb && cmb->kind == ControlKind::ComboBox);
    if (cmb) CHECK((cmb->style & 0x0003u) != 0);

    // Progress bar: unknown class -> Custom; APP_CUSTOM_STYLE unresolved,
    // recorded and NOT dropped.
    const ControlDesc* prog = find(d, 1008);
    CHECK(prog && prog->kind == ControlKind::Custom);
    if (prog) {
        CHECK(prog->windowClass == "msctls_progress32");
        bool kept = false;
        for (const auto& u : prog->unresolvedStyles)
            if (u == "APP_CUSTOM_STYLE") kept = true;
        CHECK(kept);
        CHECK((prog->style & 0x0001u) != 0);   // PBS_SMOOTH still resolved
    }

    // --- RC `NOT STYLE` + no-runaway guard ------------------------------
    const DialogDesc& dn = res.dialogs[1];
    CHECK(dn.idName == "IDD_NOTTEST");
    const ControlDesc* hidden = find(dn, 1102);   // IDC_HIDDEN_BTN
    CHECK(hidden != nullptr);
    if (hidden) {
        // NOT WS_VISIBLE removed the bit; WS_TABSTOP remains.
        CHECK((hidden->style & 0x10000000u) == 0);  // WS_VISIBLE cleared
        CHECK((hidden->style & 0x00010000u) != 0);  // WS_TABSTOP kept
    }
    const ControlDesc* ne = find(dn, 1001);        // IDC_NAME_EDIT (reused)
    CHECK(ne != nullptr);
    if (ne) {
        CHECK((ne->style & 0x80u) != 0);            // ES_AUTOHSCROLL kept
        CHECK((ne->style & 0x00800000u) == 0);      // NOT WS_BORDER
        CHECK((ne->style & 0x00010000u) == 0);      // NOT WS_TABSTOP
    }
    // IDD_AFTER must survive: if NOT desynced the parser, it would have been
    // eaten by IDD_NOTTEST's runaway (the real emule.rc failure mode).
    const DialogDesc& da = res.dialogs[2];
    CHECK(da.idName == "IDD_AFTER");
    CHECK(da.controls.size() == 1);
    CHECK(find(da, 1103) != nullptr);              // IDC_AFTER_LABEL

    // --- GDI-independent common controls (drives the Qt widget mapping) ----
    const DialogDesc& dc = res.dialogs[3];
    CHECK(dc.idName == "IDD_CONTROLS");
    CHECK(dc.controls.size() == 5);
    const ControlDesc* sld = find(dc, 1201);       // IDC_SLIDER
    CHECK(sld && sld->kind == ControlKind::Custom && sld->windowClass == "msctls_trackbar32");
    const ControlDesc* spn = find(dc, 1203);       // IDC_SPIN
    CHECK(spn && spn->kind == ControlKind::Custom && spn->windowClass == "msctls_updown32");
    // Radio group: IDC_RADIO1 carries WS_GROUP (0x20000), IDC_RADIO2 does not,
    // so DDX_Radio's group walk stops after the two of them.
    const ControlDesc* r1 = find(dc, 1204);
    const ControlDesc* r2 = find(dc, 1205);
    CHECK(r1 && r1->kind == ControlKind::RadioButton && (r1->style & 0x00020000u) != 0);
    CHECK(r2 && r2->kind == ControlKind::RadioButton && (r2->style & 0x00020000u) == 0);

    // --- emitter --------------------------------------------------------
    std::string cpp = EmitGeneratedCpp(res.dialogs, "test_dialog.rc");
    CHECK(cpp.find("#include \"dialog_ir.h\"") != std::string::npos);
    CHECK(cpp.find("RegisterDialog(&") != std::string::npos);
    CHECK(cpp.find("ControlKind::Custom") != std::string::npos);
    CHECK(cpp.find("SysListView32") != std::string::npos);

    if (g_failures == 0)
        std::printf("rc_compiler_test: parser + symbols + emitter OK "
                    "(4 dialogs incl. NOT-style + no-runaway guard + controls)\n");
    return g_failures == 0 ? 0 : 1;
}
