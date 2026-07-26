# gui/core — toolkit-agnostic GUI runtime (Layer 2b)

This directory holds the **implementation** of simple_mfc's GUI/threading
runtime that is **not** part of the frozen MFC/ATL interface in `include/`.

Hard rule (see the project plan): everything under `include/` is a faithful
**subset mirror of real ATL/MFC** and must stay identical to it — its macro
bodies, enums and signatures track real MFC, never a simple_mfc invention.
The *behaviour* behind those declarations lives here instead:

- **Message-map dispatch** — walks the `AFX_MSGMAP` chain produced by the
  (MFC-faithful) `ON_*` macros and invokes handlers by switching on the
  real-MFC `AfxSig` tag. `CCmdTarget`/`CWnd` base message maps live here.
- **Threading** — `CWinThread` / `AfxBeginThread` / `AfxGetApp` over
  `std::thread` (Milestone 1, step 0). Toolkit-agnostic std C++.
- **GUI-thread marshalling** — the `Afx_InvokeOnGuiThread` port, implemented
  by the active driver (Qt/wx). Declared minimally, default impl for tests.

Built only when `-DSIMPLE_MFC_GUI` is `qt` or `wx` (default `none` leaves the
current compile-check untouched). Toolkit-specific widget construction lives
one layer up, in `gui/qt/` and `gui/wx/`; this layer stays pure std C++.
