// gui/core/rc/rc_compiler.h — simple_mfc's build-time .rc resource compiler.
//
// This is a GENERAL framework brick (not eMule-specific): it reads a Windows
// resource script (.rc) plus its resource.h symbol header at BUILD time and
// produces the neutral dialog IR (gui/core/dialog_ir.h), emitted as generated
// C++ that is compiled and linked into the binary. It mirrors how Windows
// itself works — rc.exe -> .res -> binary, runtime only reads a precompiled
// template — so the parser runs once at build time, never per launch.
//
// The API is split so the parser is unit-testable without going through the
// command-line tool: ParseResourceScript turns text into DialogDescs, and
// EmitGeneratedCpp turns DialogDescs into a C++ source string.
#pragma once

#include "../dialog_ir.h"

#include <map>
#include <string>
#include <vector>

namespace smfc { namespace rc {

// Symbol table: identifier -> integer value, as gathered from resource.h.
using SymbolTable = std::map<std::string, long>;

struct ParseResult {
    std::vector<DialogDesc> dialogs;
    std::vector<std::string> warnings;   // non-fatal (unknown keyword, etc.)
    std::string error;                   // non-empty => fatal parse failure
    bool ok() const { return error.empty(); }
};

// Parse the `#define NAME value` lines out of a resource.h-style header.
// Values may be decimal or hex; simple `A + B` / `A - B` expressions over
// earlier symbols and literals are supported (enough for resource.h idioms).
SymbolTable ParseSymbolHeader(const std::string& headerText);

// Parse every DIALOG/DIALOGEX template in an .rc script. `symbols` resolves
// symbolic ids/styles; app-defined styles absent from the built-in Win32
// table fall back to it, and anything still unresolved is recorded verbatim.
ParseResult ParseResourceScript(const std::string& rcText,
                                const SymbolTable& symbols);

// Emit a self-contained C++ translation unit that defines the given dialogs
// as static smfc::DialogDesc constants and registers them at static-init.
std::string EmitGeneratedCpp(const std::vector<DialogDesc>& dialogs,
                             const std::string& sourceName);

}} // namespace smfc::rc
