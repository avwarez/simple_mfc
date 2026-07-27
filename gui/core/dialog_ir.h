// gui/core/dialog_ir.h — the NEUTRAL, toolkit-agnostic dialog IR.
//
// This is the compile-time product of the .rc resource compiler (see
// gui/core/rc/): for every DIALOGEX in an app's .rc, the compiler emits a
// generated .cpp that fills in these structures and registers them. At run
// time the active driver (Qt today, wx later) walks this already-in-memory
// IR to build the real widgets with the correct IDC_ ids and geometry.
//
// It lives in gui/core (Layer 2b) on purpose: it is NOT part of the frozen
// MFC/ATL interface in include/ (it is a library-internal mechanism), and it
// is consumed IDENTICALLY by every driver, so the resource work is never
// redone per toolkit. Coordinates stay in DIALOG UNITS exactly as authored;
// the driver converts to pixels at construction (MapDialogRect), because that
// needs the dialog font metrics which only exist at run time.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace smfc {

// Neutral control kind. Distinct from the raw Win32 window class so a driver
// can map it to its own widget without re-parsing styles. `Custom` carries an
// explicit window-class string (e.g. "SysListView32") for generic CONTROLs.
enum class ControlKind {
    Unknown = 0,
    Button,         // PUSHBUTTON
    DefButton,      // DEFPUSHBUTTON
    CheckBox,       // (AUTO)CHECKBOX
    RadioButton,    // (AUTO)RADIOBUTTON
    GroupBox,       // GROUPBOX
    Static,         // LTEXT / RTEXT / CTEXT
    StaticIcon,     // ICON
    Edit,           // EDITTEXT
    ListBox,        // LISTBOX
    ComboBox,       // COMBOBOX
    ScrollBar,      // SCROLLBAR
    Custom,         // generic CONTROL "..", id, "WndClass", ..
};

// One control inside a dialog template. Geometry is in dialog units.
struct ControlDesc {
    ControlKind kind = ControlKind::Unknown;
    int         id = 0;
    std::string idName;       // symbolic id, e.g. "IDC_EDIT1" (for diagnostics)
    std::string text;         // caption; empty for EDITTEXT/LISTBOX/etc.
    std::string windowClass;  // set only for Custom (raw .rc class string)
    int x = 0, y = 0, cx = 0, cy = 0;
    uint32_t style = 0;       // OR of style tokens we could resolve
    uint32_t exStyle = 0;
    // Style tokens the compiler did not recognise (kept, never dropped, so a
    // driver or a human can see exactly what was lost).
    std::vector<std::string> unresolvedStyles;
};

// One dialog template.
struct DialogDesc {
    int         id = 0;
    std::string idName;       // "IDD_FOO"
    std::string caption;
    int x = 0, y = 0, cx = 0, cy = 0;
    int         fontSize = 8;
    std::string fontFace = "MS Shell Dlg";
    uint32_t    style = 0;
    uint32_t    exStyle = 0;
    std::vector<ControlDesc> controls;
};

// --- Runtime registry (generated code registers; drivers look up) ----------
// RegisterDialog is called from the generated .cpp at static-init; it stores
// the pointer in a function-local static (no static-init-order hazard). The
// DialogDesc objects themselves are generated as static constants, so the
// registry only holds borrowed pointers with program lifetime.
void RegisterDialog(const DialogDesc* d);

// Look up a dialog template by its numeric resource id (IDD_*). Returns
// nullptr if no .rc supplied that dialog.
const DialogDesc* FindDialog(int id);

// All registered dialogs, in registration order.
const std::vector<const DialogDesc*>& AllDialogs();

} // namespace smfc
