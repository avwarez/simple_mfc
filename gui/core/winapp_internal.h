// gui/core/winapp_internal.h — the seam between the framework entry point
// (driver-side, since it must create the toolkit's application object) and
// CWinApp's toolkit-independent state (gui/core). Library-internal: not part
// of the frozen MFC interface.
#pragma once

#include <string>
#include <vector>

namespace smfc {

// Hand the process command line to CWinApp::ParseCommandLine. Called by
// AfxWinMain before InitInstance, which is where MFC's own command line is
// already available. The program name (argv[0]) is NOT included: real MFC's
// ParseCommandLine skips it, and eMule's ParseParam override assumes that.
void SetCommandLine(const std::vector<std::wstring>& args);

} // namespace smfc
