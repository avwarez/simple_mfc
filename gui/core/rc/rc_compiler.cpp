// gui/core/rc/rc_compiler.cpp — implementation of simple_mfc's .rc compiler.
// std-only; see rc_compiler.h for the design and the framework rationale.
#include "rc_compiler.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace smfc { namespace rc {

namespace {

// --- Built-in Win32 style constants ---------------------------------------
// Only the window/control styles a dialog template actually references. Real
// MFC pulls these from <winuser.h>/<commctrl.h>; we carry the values so the
// compiler needs no Windows SDK. Unknown tokens are not an error: they are
// resolved against the app's own SymbolTable, and failing that recorded in
// ControlDesc::unresolvedStyles so nothing is silently dropped.
const std::unordered_map<std::string, uint32_t>& styleConstants()
{
    static const std::unordered_map<std::string, uint32_t> k = {
        // WS_*
        {"WS_OVERLAPPED", 0x00000000u}, {"WS_POPUP", 0x80000000u},
        {"WS_CHILD", 0x40000000u},      {"WS_MINIMIZE", 0x20000000u},
        {"WS_VISIBLE", 0x10000000u},    {"WS_DISABLED", 0x08000000u},
        {"WS_CLIPSIBLINGS", 0x04000000u}, {"WS_CLIPCHILDREN", 0x02000000u},
        {"WS_MAXIMIZE", 0x01000000u},   {"WS_CAPTION", 0x00C00000u},
        {"WS_BORDER", 0x00800000u},     {"WS_DLGFRAME", 0x00400000u},
        {"WS_VSCROLL", 0x00200000u},    {"WS_HSCROLL", 0x00100000u},
        {"WS_SYSMENU", 0x00080000u},    {"WS_THICKFRAME", 0x00040000u},
        {"WS_GROUP", 0x00020000u},      {"WS_TABSTOP", 0x00010000u},
        {"WS_MINIMIZEBOX", 0x00020000u}, {"WS_MAXIMIZEBOX", 0x00010000u},
        // WS_EX_*
        {"WS_EX_DLGMODALFRAME", 0x00000001u}, {"WS_EX_TOPMOST", 0x00000008u},
        {"WS_EX_ACCEPTFILES", 0x00000010u}, {"WS_EX_TRANSPARENT", 0x00000020u},
        {"WS_EX_TOOLWINDOW", 0x00000080u}, {"WS_EX_WINDOWEDGE", 0x00000100u},
        {"WS_EX_CLIENTEDGE", 0x00000200u}, {"WS_EX_CONTEXTHELP", 0x00000400u},
        {"WS_EX_RIGHT", 0x00001000u},   {"WS_EX_LEFT", 0x00000000u},
        {"WS_EX_STATICEDGE", 0x00020000u}, {"WS_EX_APPWINDOW", 0x00040000u},
        {"WS_EX_LAYERED", 0x00080000u}, {"WS_EX_NOACTIVATE", 0x08000000u},
        // DS_* (dialog styles)
        {"DS_SETFONT", 0x0040u}, {"DS_MODALFRAME", 0x0080u},
        {"DS_FIXEDSYS", 0x0008u}, {"DS_CENTER", 0x0800u},
        {"DS_SHELLFONT", 0x0048u}, {"DS_CONTROL", 0x0400u},
        {"DS_3DLOOK", 0x0004u}, {"DS_NOIDLEMSG", 0x0100u},
        // BS_* (button)
        {"BS_PUSHBUTTON", 0x00u}, {"BS_DEFPUSHBUTTON", 0x01u},
        {"BS_CHECKBOX", 0x02u}, {"BS_AUTOCHECKBOX", 0x03u},
        {"BS_RADIOBUTTON", 0x04u}, {"BS_3STATE", 0x05u},
        {"BS_AUTO3STATE", 0x06u}, {"BS_GROUPBOX", 0x07u},
        {"BS_USERBUTTON", 0x08u}, {"BS_AUTORADIOBUTTON", 0x09u},
        {"BS_OWNERDRAW", 0x0Bu}, {"BS_LEFTTEXT", 0x20u},
        {"BS_TEXT", 0x00u}, {"BS_ICON", 0x40u}, {"BS_BITMAP", 0x80u},
        {"BS_LEFT", 0x0100u}, {"BS_RIGHT", 0x0200u}, {"BS_CENTER", 0x0300u},
        {"BS_TOP", 0x0400u}, {"BS_BOTTOM", 0x0800u}, {"BS_VCENTER", 0x0C00u},
        {"BS_PUSHLIKE", 0x1000u}, {"BS_MULTILINE", 0x2000u},
        {"BS_NOTIFY", 0x4000u}, {"BS_FLAT", 0x8000u},
        // ES_* (edit)
        {"ES_LEFT", 0x0000u}, {"ES_CENTER", 0x0001u}, {"ES_RIGHT", 0x0002u},
        {"ES_MULTILINE", 0x0004u}, {"ES_UPPERCASE", 0x0008u},
        {"ES_LOWERCASE", 0x0010u}, {"ES_PASSWORD", 0x0020u},
        {"ES_AUTOVSCROLL", 0x0040u}, {"ES_AUTOHSCROLL", 0x0080u},
        {"ES_NOHIDESEL", 0x0100u}, {"ES_OEMCONVERT", 0x0400u},
        {"ES_READONLY", 0x0800u}, {"ES_WANTRETURN", 0x1000u},
        {"ES_NUMBER", 0x2000u},
        // SS_* (static)
        {"SS_LEFT", 0x0000u}, {"SS_CENTER", 0x0001u}, {"SS_RIGHT", 0x0002u},
        {"SS_ICON", 0x0003u}, {"SS_BLACKRECT", 0x0004u},
        {"SS_GRAYRECT", 0x0005u}, {"SS_WHITERECT", 0x0006u},
        {"SS_BLACKFRAME", 0x0007u}, {"SS_ETCHEDHORZ", 0x0010u},
        {"SS_ETCHEDVERT", 0x0011u}, {"SS_ETCHEDFRAME", 0x0012u},
        {"SS_LEFTNOWORDWRAP", 0x000Cu}, {"SS_BITMAP", 0x000Eu},
        {"SS_OWNERDRAW", 0x000Du}, {"SS_ENHMETAFILE", 0x000Fu},
        {"SS_NOPREFIX", 0x0080u}, {"SS_NOTIFY", 0x0100u},
        {"SS_CENTERIMAGE", 0x0200u}, {"SS_RIGHTJUST", 0x0400u},
        {"SS_REALSIZEIMAGE", 0x0800u}, {"SS_SUNKEN", 0x1000u},
        {"SS_ENDELLIPSIS", 0x4000u}, {"SS_PATHELLIPSIS", 0x8000u},
        // ES/SS shared: none
        // CBS_* (combo)
        {"CBS_SIMPLE", 0x0001u}, {"CBS_DROPDOWN", 0x0002u},
        {"CBS_DROPDOWNLIST", 0x0003u}, {"CBS_OWNERDRAWFIXED", 0x0010u},
        {"CBS_OWNERDRAWVARIABLE", 0x0020u}, {"CBS_AUTOHSCROLL", 0x0040u},
        {"CBS_SORT", 0x0100u}, {"CBS_HASSTRINGS", 0x0200u},
        {"CBS_NOINTEGRALHEIGHT", 0x0400u}, {"CBS_DISABLENOSCROLL", 0x0800u},
        // LBS_* (listbox)
        {"LBS_NOTIFY", 0x0001u}, {"LBS_SORT", 0x0002u},
        {"LBS_NOREDRAW", 0x0004u}, {"LBS_MULTIPLESEL", 0x0008u},
        {"LBS_OWNERDRAWFIXED", 0x0010u}, {"LBS_OWNERDRAWVARIABLE", 0x0020u},
        {"LBS_HASSTRINGS", 0x0040u}, {"LBS_USETABSTOPS", 0x0080u},
        {"LBS_NOINTEGRALHEIGHT", 0x0100u}, {"LBS_MULTICOLUMN", 0x0200u},
        {"LBS_WANTKEYBOARDINPUT", 0x0400u}, {"LBS_EXTENDEDSEL", 0x0800u},
        {"LBS_DISABLENOSCROLL", 0x1000u}, {"LBS_NODATA", 0x2000u},
        // LVS_* (list view)
        {"LVS_ICON", 0x0000u}, {"LVS_REPORT", 0x0001u},
        {"LVS_SMALLICON", 0x0002u}, {"LVS_LIST", 0x0003u},
        {"LVS_SINGLESEL", 0x0004u}, {"LVS_SHOWSELALWAYS", 0x0008u},
        {"LVS_SORTASCENDING", 0x0010u}, {"LVS_SORTDESCENDING", 0x0020u},
        {"LVS_SHAREIMAGELISTS", 0x0040u}, {"LVS_NOLABELWRAP", 0x0080u},
        {"LVS_AUTOARRANGE", 0x0100u}, {"LVS_EDITLABELS", 0x0200u},
        {"LVS_OWNERDATA", 0x1000u}, {"LVS_NOSCROLL", 0x2000u},
        {"LVS_ALIGNTOP", 0x0000u}, {"LVS_ALIGNLEFT", 0x0800u},
        {"LVS_OWNERDRAWFIXED", 0x0400u}, {"LVS_NOCOLUMNHEADER", 0x4000u},
        {"LVS_NOSORTHEADER", 0x8000u},
        // TVS_* (tree view)
        {"TVS_HASBUTTONS", 0x0001u}, {"TVS_HASLINES", 0x0002u},
        {"TVS_LINESATROOT", 0x0004u}, {"TVS_EDITLABELS", 0x0008u},
        {"TVS_DISABLEDRAGDROP", 0x0010u}, {"TVS_SHOWSELALWAYS", 0x0020u},
        {"TVS_CHECKBOXES", 0x0100u}, {"TVS_TRACKSELECT", 0x0200u},
        {"TVS_SINGLEEXPAND", 0x0400u}, {"TVS_FULLROWSELECT", 0x1000u},
        {"TVS_NOSCROLL", 0x2000u}, {"TVS_NONEVENHEIGHT", 0x4000u},
        // TCS_* (tab)
        {"TCS_TABS", 0x0000u}, {"TCS_BUTTONS", 0x0100u},
        {"TCS_MULTILINE", 0x0200u}, {"TCS_FIXEDWIDTH", 0x0400u},
        // Slider / progress / spin
        {"TBS_HORZ", 0x0000u}, {"TBS_VERT", 0x0002u},
        {"TBS_AUTOTICKS", 0x0001u}, {"TBS_BOTH", 0x0008u},
        {"TBS_NOTICKS", 0x0010u}, {"TBS_ENABLESELRANGE", 0x0020u},
        {"PBS_SMOOTH", 0x0001u}, {"PBS_VERTICAL", 0x0004u},
        {"UDS_WRAP", 0x0001u}, {"UDS_SETBUDDYINT", 0x0002u},
        {"UDS_ALIGNRIGHT", 0x0004u}, {"UDS_ALIGNLEFT", 0x0008u},
        {"UDS_AUTOBUDDY", 0x0010u}, {"UDS_ARROWKEYS", 0x0020u},
        {"UDS_HORZ", 0x0040u}, {"UDS_NOTHOUSANDS", 0x0080u},
        // SBS_* (scrollbar)
        {"SBS_HORZ", 0x0000u}, {"SBS_VERT", 0x0001u},
    };
    return k;
}

// Standard resource ids that a real rc.exe gets from <winres.h>/<winuser.h>,
// not from the app's resource.h: the common-dialog button ids and IDC_STATIC.
// Seeded as built-ins so IDOK/IDCANCEL/... resolve even when the app header
// does not (re)define them. The app's own SymbolTable still takes precedence.
const std::unordered_map<std::string, long>& standardIds()
{
    static const std::unordered_map<std::string, long> k = {
        {"IDOK", 1}, {"IDCANCEL", 2}, {"IDABORT", 3}, {"IDRETRY", 4},
        {"IDIGNORE", 5}, {"IDYES", 6}, {"IDNO", 7}, {"IDCLOSE", 8},
        {"IDHELP", 9}, {"IDTRYAGAIN", 10}, {"IDCONTINUE", 11},
        {"IDC_STATIC", -1},
    };
    return k;
}

// The set of statement keywords that start a control inside BEGIN..END, plus
// END. Used to detect statement boundaries while parsing optional trailing
// style/exstyle fields (a style token is never one of these reserved words).
const std::unordered_set<std::string>& controlKeywords()
{
    static const std::unordered_set<std::string> k = {
        "LTEXT", "RTEXT", "CTEXT", "ICON",
        "PUSHBUTTON", "DEFPUSHBUTTON", "PUSHBOX",
        "CHECKBOX", "AUTOCHECKBOX", "STATE3", "AUTO3STATE",
        "RADIOBUTTON", "AUTORADIOBUTTON",
        "GROUPBOX", "USERBUTTON",
        "EDITTEXT", "LISTBOX", "COMBOBOX", "SCROLLBAR",
        "CONTROL", "END",
    };
    return k;
}

// --- Tokeniser ------------------------------------------------------------
enum class TokKind { Id, Number, String, Op, End };

struct Token {
    TokKind kind = TokKind::End;
    std::string text;   // identifier / operator / raw number text
    std::string str;    // decoded string contents (String tokens)
    long number = 0;    // parsed value (Number tokens)
};

// Strip // and /* */ comments, preserving string literals.
std::string stripComments(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        char c = s[i];
        if (c == '"') {                 // copy a whole string literal verbatim
            out += c;
            ++i;
            while (i < s.size()) {
                out += s[i];
                if (s[i] == '"') {
                    if (i + 1 < s.size() && s[i + 1] == '"') { // "" escape
                        out += s[i + 1];
                        i += 2;
                        continue;
                    }
                    ++i;
                    break;
                }
                ++i;
            }
            continue;
        }
        if (c == '/' && i + 1 < s.size() && s[i + 1] == '/') {
            while (i < s.size() && s[i] != '\n') ++i;
            continue;
        }
        if (c == '/' && i + 1 < s.size() && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < s.size() && !(s[i] == '*' && s[i + 1] == '/')) ++i;
            i += 2;
            continue;
        }
        out += c;
        ++i;
    }
    return out;
}

bool isIdStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }
bool isIdChar(char c) { return std::isalnum((unsigned char)c) || c == '_'; }

class Lexer {
public:
    explicit Lexer(const std::string& s) : s_(s) {}

    Token next()
    {
        skipSpace();
        if (pos_ >= s_.size()) return {TokKind::End, "", "", 0};
        char c = s_[pos_];
        if (c == '"') return lexString();
        if (isIdStart(c)) return lexId();
        if (std::isdigit((unsigned char)c) ||
            (c == '-' && pos_ + 1 < s_.size() &&
             std::isdigit((unsigned char)s_[pos_ + 1])))
            return lexNumber();
        // single-char operator: , | + - ( ) etc.
        ++pos_;
        return {TokKind::Op, std::string(1, c), "", 0};
    }

private:
    void skipSpace()
    {
        while (pos_ < s_.size() && std::isspace((unsigned char)s_[pos_]))
            ++pos_;
    }

    Token lexId()
    {
        size_t start = pos_;
        while (pos_ < s_.size() && isIdChar(s_[pos_])) ++pos_;
        return {TokKind::Id, s_.substr(start, pos_ - start), "", 0};
    }

    Token lexNumber()
    {
        size_t start = pos_;
        if (s_[pos_] == '-') ++pos_;
        int base = 10;
        if (pos_ + 1 < s_.size() && s_[pos_] == '0' &&
            (s_[pos_ + 1] == 'x' || s_[pos_ + 1] == 'X')) {
            base = 16;
            pos_ += 2;
            while (pos_ < s_.size() && std::isxdigit((unsigned char)s_[pos_]))
                ++pos_;
        } else {
            while (pos_ < s_.size() && std::isdigit((unsigned char)s_[pos_]))
                ++pos_;
        }
        // trailing L/U suffixes on constants
        while (pos_ < s_.size() &&
               (s_[pos_] == 'L' || s_[pos_] == 'l' ||
                s_[pos_] == 'U' || s_[pos_] == 'u'))
            ++pos_;
        std::string raw = s_.substr(start, pos_ - start);
        std::string clean = raw;
        while (!clean.empty() && (clean.back() == 'L' || clean.back() == 'l' ||
                                  clean.back() == 'U' || clean.back() == 'u'))
            clean.pop_back();
        Token t{TokKind::Number, raw, "", 0};
        t.number = std::strtol(clean.c_str(), nullptr, base);
        return t;
    }

    Token lexString()
    {
        ++pos_; // opening quote
        std::string val;
        while (pos_ < s_.size()) {
            char c = s_[pos_];
            if (c == '"') {
                if (pos_ + 1 < s_.size() && s_[pos_ + 1] == '"') { // "" -> "
                    val += '"';
                    pos_ += 2;
                    continue;
                }
                ++pos_;
                break;
            }
            if (c == '\\' && pos_ + 1 < s_.size()) {  // basic C escapes
                char n = s_[pos_ + 1];
                switch (n) {
                    case 'n': val += '\n'; break;
                    case 't': val += '\t'; break;
                    case 'r': val += '\r'; break;
                    case '\\': val += '\\'; break;
                    case '"': val += '"'; break;
                    default: val += n; break;
                }
                pos_ += 2;
                continue;
            }
            val += c;
            ++pos_;
        }
        return {TokKind::String, val, val, 0};
    }

    const std::string& s_;
    size_t pos_ = 0;
};

// Tokenise the whole (comment-stripped) input into a flat vector so the parser
// can look ahead freely.
std::vector<Token> tokenize(const std::string& text)
{
    Lexer lex(text);
    std::vector<Token> out;
    for (;;) {
        Token t = lex.next();
        if (t.kind == TokKind::End) break;
        out.push_back(std::move(t));
    }
    return out;
}

// --- Parser ---------------------------------------------------------------
class Parser {
public:
    Parser(std::vector<Token> toks, const SymbolTable& syms, ParseResult& res)
        : toks_(std::move(toks)), syms_(syms), res_(res) {}

    void run()
    {
        while (i_ < toks_.size()) {
            // A dialog is:  <ID> DIALOG[EX] ...
            if (toks_[i_].kind == TokKind::Id && i_ + 1 < toks_.size() &&
                toks_[i_ + 1].kind == TokKind::Id &&
                (toks_[i_ + 1].text == "DIALOGEX" ||
                 toks_[i_ + 1].text == "DIALOG")) {
                parseDialog();
            } else {
                ++i_;   // skip anything that is not a dialog template
            }
        }
    }

private:
    const Token& cur() const { return toks_[i_]; }
    bool atEnd() const { return i_ >= toks_.size(); }
    bool isOp(char c) const
    {
        return !atEnd() && cur().kind == TokKind::Op && cur().text.size() == 1 &&
               cur().text[0] == c;
    }
    void skipCommas() { while (isOp(',')) ++i_; }

    // Resolve a symbol name to a value: built-in style table first, then the
    // app symbol table. `found` reports whether it resolved at all.
    long resolveName(const std::string& name, bool& found) const
    {
        auto s = styleConstants().find(name);
        if (s != styleConstants().end()) { found = true; return (long)s->second; }
        auto a = syms_.find(name);   // app resource.h takes precedence
        if (a != syms_.end()) { found = true; return a->second; }
        auto b = standardIds().find(name);   // then SDK-standard ids
        if (b != standardIds().end()) { found = true; return b->second; }
        found = false;
        return 0;
    }

    // Parse an integer value term: number | symbol, with + and - between terms.
    long parseIntExpr(bool* allResolved = nullptr)
    {
        long acc = 0;
        int sign = 1;
        bool resolved = true;
        for (;;) {
            if (cur().kind == TokKind::Number) {
                acc += sign * cur().number;
                ++i_;
            } else if (cur().kind == TokKind::Id) {
                bool f = false;
                long v = resolveName(cur().text, f);
                if (!f) resolved = false;
                acc += sign * v;
                ++i_;
            } else {
                break;
            }
            if (isOp('+')) { sign = 1; ++i_; continue; }
            if (isOp('-')) { sign = -1; ++i_; continue; }
            break;
        }
        if (allResolved) *allResolved = resolved;
        return acc;
    }

    // Parse a style expression (TOK | TOK | ..). Returns the OR of resolved
    // constants; unresolved token names are appended to `unresolved`.
    uint32_t parseStyleExpr(std::vector<std::string>& unresolved)
    {
        uint32_t acc = 0;
        for (;;) {
            if (cur().kind == TokKind::Number) {
                acc |= (uint32_t)cur().number;
                ++i_;
            } else if (cur().kind == TokKind::Id) {
                bool f = false;
                long v = resolveName(cur().text, f);
                if (f) acc |= (uint32_t)v;
                else unresolved.push_back(cur().text);
                ++i_;
            } else {
                break;
            }
            if (isOp('|')) { ++i_; continue; }
            break;
        }
        return acc;
    }

    // True if `t` begins a new control statement or ends the block.
    bool atStatementBoundary() const
    {
        return atEnd() ||
               (cur().kind == TokKind::Id &&
                controlKeywords().count(cur().text) > 0);
    }

    void parseDialog()
    {
        DialogDesc d;
        d.idName = toks_[i_].text;
        bool f = false;
        d.id = (int)resolveName(d.idName, f);
        i_ += 2;   // skip <ID> DIALOG[EX]
        skipCommas();
        // Geometry: x, y, cx, cy
        d.x = (int)parseIntExpr(); skipCommas();
        d.y = (int)parseIntExpr(); skipCommas();
        d.cx = (int)parseIntExpr(); skipCommas();
        d.cy = (int)parseIntExpr(); skipCommas();

        // Optional header lines until BEGIN.
        while (!atEnd()) {
            if (cur().kind == TokKind::Id && cur().text == "BEGIN") { ++i_; break; }
            if (cur().kind == TokKind::Id && cur().text == "STYLE") {
                ++i_; skipCommas();
                d.style = parseStyleExpr(scratchUnresolved_);
            } else if (cur().kind == TokKind::Id && cur().text == "EXSTYLE") {
                ++i_; skipCommas();
                d.exStyle = parseStyleExpr(scratchUnresolved_);
            } else if (cur().kind == TokKind::Id && cur().text == "CAPTION") {
                ++i_;
                if (!atEnd() && cur().kind == TokKind::String) {
                    d.caption = cur().str;
                    ++i_;
                }
            } else if (cur().kind == TokKind::Id && cur().text == "FONT") {
                ++i_; skipCommas();
                if (!atEnd() && cur().kind == TokKind::Number) {
                    d.fontSize = (int)cur().number;
                    ++i_;
                }
                skipCommas();
                if (!atEnd() && cur().kind == TokKind::String) {
                    d.fontFace = cur().str;
                    ++i_;
                }
                // skip any remaining FONT fields (weight, italic, charset)
                while (!atEnd() && !(cur().kind == TokKind::Id &&
                       (cur().text == "BEGIN" || cur().text == "STYLE" ||
                        cur().text == "CAPTION" || cur().text == "EXSTYLE" ||
                        cur().text == "FONT")))
                    ++i_;
            } else {
                ++i_;   // MENU, CLASS, etc. — skip
            }
        }

        // Controls until END.
        while (!atEnd()) {
            if (cur().kind == TokKind::Id && cur().text == "END") { ++i_; break; }
            if (cur().kind != TokKind::Id) { ++i_; continue; }
            parseControl(d);
        }

        res_.dialogs.push_back(std::move(d));
    }

    // Read an id field (symbol or number); records the symbolic name.
    void readId(ControlDesc& c)
    {
        if (cur().kind == TokKind::Id) c.idName = cur().text;
        c.id = (int)parseIntExpr();
    }

    void parseControl(DialogDesc& d)
    {
        const std::string kw = cur().text;
        ++i_;

        ControlDesc c;

        if (kw == "CONTROL") {
            // CONTROL "text", id, "class", style, x, y, cx, cy [, exstyle]
            skipCommas();
            if (cur().kind == TokKind::String) { c.text = cur().str; ++i_; }
            skipCommas();
            readId(c); skipCommas();
            if (cur().kind == TokKind::String) { c.windowClass = cur().str; ++i_; }
            skipCommas();
            c.style = parseStyleExpr(c.unresolvedStyles); skipCommas();
            c.x = (int)parseIntExpr(); skipCommas();
            c.y = (int)parseIntExpr(); skipCommas();
            c.cx = (int)parseIntExpr(); skipCommas();
            c.cy = (int)parseIntExpr(); skipCommas();
            if (!atStatementBoundary() &&
                (cur().kind == TokKind::Number || cur().kind == TokKind::Id))
                c.exStyle = parseStyleExpr(c.unresolvedStyles);
            c.kind = classifyControlClass(c.windowClass, c.style);
            d.controls.push_back(std::move(c));
            return;
        }

        const bool hasText = keywordHasText(kw);
        const bool isIcon = (kw == "ICON");
        c.kind = classifyKeyword(kw);

        skipCommas();
        if (hasText) {
            if (cur().kind == TokKind::String) { c.text = cur().str; ++i_; }
            skipCommas();
        }
        readId(c); skipCommas();
        c.x = (int)parseIntExpr(); skipCommas();
        c.y = (int)parseIntExpr(); skipCommas();

        if (isIcon) {
            // ICON has optional width/height; both or neither.
            if (!atStatementBoundary() && cur().kind == TokKind::Number) {
                c.cx = (int)parseIntExpr(); skipCommas();
                if (!atStatementBoundary() && cur().kind == TokKind::Number) {
                    c.cy = (int)parseIntExpr(); skipCommas();
                }
            }
        } else {
            c.cx = (int)parseIntExpr(); skipCommas();
            c.cy = (int)parseIntExpr(); skipCommas();
        }

        // Optional trailing style, then optional exstyle.
        if (!atStatementBoundary() &&
            (cur().kind == TokKind::Id || cur().kind == TokKind::Number)) {
            c.style |= parseStyleExpr(c.unresolvedStyles);
            skipCommas();
            if (!atStatementBoundary() &&
                (cur().kind == TokKind::Id || cur().kind == TokKind::Number))
                c.exStyle = parseStyleExpr(c.unresolvedStyles);
        }

        d.controls.push_back(std::move(c));
    }

    static bool keywordHasText(const std::string& kw)
    {
        // These control statements carry no leading text field.
        return !(kw == "EDITTEXT" || kw == "LISTBOX" ||
                 kw == "COMBOBOX" || kw == "SCROLLBAR");
    }

    static ControlKind classifyKeyword(const std::string& kw)
    {
        if (kw == "DEFPUSHBUTTON") return ControlKind::DefButton;
        if (kw == "PUSHBUTTON" || kw == "PUSHBOX" || kw == "USERBUTTON")
            return ControlKind::Button;
        if (kw == "CHECKBOX" || kw == "AUTOCHECKBOX" ||
            kw == "STATE3" || kw == "AUTO3STATE")
            return ControlKind::CheckBox;
        if (kw == "RADIOBUTTON" || kw == "AUTORADIOBUTTON")
            return ControlKind::RadioButton;
        if (kw == "GROUPBOX") return ControlKind::GroupBox;
        if (kw == "LTEXT" || kw == "RTEXT" || kw == "CTEXT")
            return ControlKind::Static;
        if (kw == "ICON") return ControlKind::StaticIcon;
        if (kw == "EDITTEXT") return ControlKind::Edit;
        if (kw == "LISTBOX") return ControlKind::ListBox;
        if (kw == "COMBOBOX") return ControlKind::ComboBox;
        if (kw == "SCROLLBAR") return ControlKind::ScrollBar;
        return ControlKind::Unknown;
    }

    // Map a generic CONTROL's window class to a neutral kind. Falls back to
    // Custom (keeping the class string) for anything not recognised.
    static ControlKind classifyControlClass(const std::string& cls, uint32_t style)
    {
        std::string c;
        for (char ch : cls) c += (char)std::tolower((unsigned char)ch);
        if (c == "button") {
            uint32_t bs = style & 0x0Fu;
            if (bs == 0x03u || bs == 0x02u) return ControlKind::CheckBox;
            if (bs == 0x09u || bs == 0x04u) return ControlKind::RadioButton;
            if (bs == 0x07u) return ControlKind::GroupBox;
            if (bs == 0x01u) return ControlKind::DefButton;
            return ControlKind::Button;
        }
        if (c == "edit") return ControlKind::Edit;
        if (c == "static") return ControlKind::Static;
        if (c == "listbox") return ControlKind::ListBox;
        if (c == "combobox") return ControlKind::ComboBox;
        if (c == "scrollbar") return ControlKind::ScrollBar;
        return ControlKind::Custom;   // SysListView32, SysTreeView32, etc.
    }

    std::vector<Token> toks_;
    const SymbolTable& syms_;
    ParseResult& res_;
    size_t i_ = 0;
    std::vector<std::string> scratchUnresolved_;  // dialog-level style leftovers
};

// --- Emitter helpers ------------------------------------------------------
std::string cppEscape(const std::string& s)
{
    std::string out;
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

const char* kindName(ControlKind k)
{
    switch (k) {
        case ControlKind::Button: return "Button";
        case ControlKind::DefButton: return "DefButton";
        case ControlKind::CheckBox: return "CheckBox";
        case ControlKind::RadioButton: return "RadioButton";
        case ControlKind::GroupBox: return "GroupBox";
        case ControlKind::Static: return "Static";
        case ControlKind::StaticIcon: return "StaticIcon";
        case ControlKind::Edit: return "Edit";
        case ControlKind::ListBox: return "ListBox";
        case ControlKind::ComboBox: return "ComboBox";
        case ControlKind::ScrollBar: return "ScrollBar";
        case ControlKind::Custom: return "Custom";
        case ControlKind::Unknown: default: return "Unknown";
    }
}

// A C++ identifier fragment safe for a variable name (dialogs keyed by id).
std::string safeIdent(const std::string& name, int fallbackId)
{
    std::string out;
    for (char c : name)
        out += isIdChar(c) ? c : '_';
    if (out.empty() || std::isdigit((unsigned char)out[0]))
        out = "d" + std::to_string(fallbackId) + "_" + out;
    return out;
}

} // namespace

SymbolTable ParseSymbolHeader(const std::string& headerText)
{
    SymbolTable syms;
    std::string clean = stripComments(headerText);
    std::istringstream in(clean);
    std::string line;
    while (std::getline(in, line)) {
        // Look for: #define NAME value
        size_t p = line.find('#');
        if (p == std::string::npos) continue;
        size_t d = line.find("define", p);
        if (d == std::string::npos) continue;
        size_t q = d + 6;
        while (q < line.size() && std::isspace((unsigned char)line[q])) ++q;
        size_t nameStart = q;
        while (q < line.size() && isIdChar(line[q])) ++q;
        if (q == nameStart) continue;
        std::string name = line.substr(nameStart, q - nameStart);
        // reject function-like macros: NAME(
        if (q < line.size() && line[q] == '(') continue;
        std::string rest = line.substr(q);
        // Evaluate the value as an int expression over earlier symbols.
        std::vector<Token> toks = tokenize(rest);
        if (toks.empty()) continue;
        long acc = 0;
        int sign = 1;
        bool any = false;
        for (size_t k = 0; k < toks.size(); ++k) {
            const Token& t = toks[k];
            if (t.kind == TokKind::Number) { acc += sign * t.number; any = true; }
            else if (t.kind == TokKind::Id) {
                auto it = syms.find(t.text);
                if (it != syms.end()) { acc += sign * it->second; any = true; }
                else { any = false; break; }   // unresolved -> skip this define
            } else if (t.kind == TokKind::Op && t.text == "+") sign = 1;
            else if (t.kind == TokKind::Op && t.text == "-") sign = -1;
            else if (t.kind == TokKind::Op && (t.text == "(" || t.text == ")"))
                continue;
            else { any = false; break; }
        }
        if (any) syms[name] = acc;
    }
    return syms;
}

ParseResult ParseResourceScript(const std::string& rcText,
                                const SymbolTable& symbols)
{
    ParseResult res;
    std::string clean = stripComments(rcText);
    std::vector<Token> toks = tokenize(clean);
    Parser p(std::move(toks), symbols, res);
    p.run();
    // Collect warnings for controls carrying unresolved styles.
    for (const auto& d : res.dialogs)
        for (const auto& c : d.controls)
            for (const auto& u : c.unresolvedStyles)
                res.warnings.push_back(d.idName + ": control " + c.idName +
                                       " has unresolved style '" + u + "'");
    return res;
}

std::string EmitGeneratedCpp(const std::vector<DialogDesc>& dialogs,
                             const std::string& sourceName)
{
    std::ostringstream o;
    o << "// GENERATED by simple_mfc's .rc resource compiler. DO NOT EDIT.\n";
    o << "// source: " << sourceName << "\n";
    o << "#include \"dialog_ir.h\"\n\n";
    o << "namespace {\nusing namespace smfc;\n\n";

    for (const auto& d : dialogs) {
        std::string var = safeIdent(d.idName, d.id);
        o << "const DialogDesc " << var << " = {\n";
        o << "    " << d.id << ", \"" << cppEscape(d.idName) << "\", \""
          << cppEscape(d.caption) << "\",\n";
        o << "    " << d.x << ", " << d.y << ", " << d.cx << ", " << d.cy << ",\n";
        o << "    " << d.fontSize << ", \"" << cppEscape(d.fontFace) << "\",\n";
        o << "    " << d.style << "u, " << d.exStyle << "u,\n";
        o << "    {\n";
        for (const auto& c : d.controls) {
            o << "        { ControlKind::" << kindName(c.kind) << ", "
              << c.id << ", \"" << cppEscape(c.idName) << "\", \""
              << cppEscape(c.text) << "\", \"" << cppEscape(c.windowClass)
              << "\", " << c.x << ", " << c.y << ", " << c.cx << ", " << c.cy
              << ", " << c.style << "u, " << c.exStyle << "u, {";
            for (size_t k = 0; k < c.unresolvedStyles.size(); ++k)
                o << (k ? ", " : " ") << "\"" << cppEscape(c.unresolvedStyles[k])
                  << "\"";
            o << (c.unresolvedStyles.empty() ? "} },\n" : " } },\n");
        }
        o << "    }\n};\n\n";
    }

    o << "struct AutoRegister {\n    AutoRegister() {\n";
    for (const auto& d : dialogs)
        o << "        RegisterDialog(&" << safeIdent(d.idName, d.id) << ");\n";
    o << "    }\n};\nconst AutoRegister g_autoRegister;\n\n";
    o << "} // namespace\n";
    return o.str();
}

}} // namespace smfc::rc
