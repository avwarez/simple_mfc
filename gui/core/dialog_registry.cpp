// gui/core/dialog_registry.cpp — the neutral dialog-IR registry.
//
// Generated resource code (from gui/core/rc/) calls RegisterDialog at static
// initialisation; drivers call FindDialog/AllDialogs at run time. The backing
// store is a function-local static so registration works regardless of the
// static-initialisation order across translation units.
#include "dialog_ir.h"

#include <unordered_map>

namespace smfc {

namespace {
std::vector<const DialogDesc*>& registry()
{
    static std::vector<const DialogDesc*> r;
    return r;
}

std::unordered_map<int, const DialogDesc*>& byId()
{
    static std::unordered_map<int, const DialogDesc*> m;
    return m;
}
} // namespace

void RegisterDialog(const DialogDesc* d)
{
    if (!d)
        return;
    registry().push_back(d);
    byId()[d->id] = d;   // last one wins if an id is somehow registered twice
}

const DialogDesc* FindDialog(int id)
{
    auto& m = byId();
    auto it = m.find(id);
    return it == m.end() ? nullptr : it->second;
}

const std::vector<const DialogDesc*>& AllDialogs()
{
    return registry();
}

} // namespace smfc
