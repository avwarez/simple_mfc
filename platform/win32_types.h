// win32_types.h -- GENERATED, do not edit by hand.
//
// Struct layouts, enums and typedefs lifted verbatim from the preprocessed
// mingw-w64 headers, so field order and field types cannot drift from the SDK.
// Regenerate with tools/win32_oracle/mktypes.py.
//
// Three properties make this safe here:
//   * The preprocessor expands macros but NOT typedefs, so fields come out as
//     `LONG lfHeight`, and afx.h already owns LONG at the correct 32-bit
//     width. Emitting the underlying `long` would be 8 bytes on LP64 and 4 on
//     Windows.
//   * Where the SDK does spell a base type outright -- GUID's
//     `unsigned long Data1` -- the generator narrows it to `int`, which is
//     32-bit on both. Otherwise every field after it would shift.
//   * The oracle is preprocessed with UNICODE defined, matching how eMule
//     builds, so LPLOGFONT resolves to LOGFONTW and not LOGFONTA.
//
// Only what eMule failed on is emitted, plus what it transitively needs.
// Names owned by afx.h / atltypes.h / atlcomcli.h, and names that belong to
// glibc on this platform, are deliberately absent: single owner per symbol.

#pragma once

// GUID's owner is atlcomcli.h -- it is a COM type, and that header is
// reachable without afxwin.h, so it cannot be the one to borrow. Several
// structures here (NOTIFYICONDATAW.guidItem) have a GUID field, so pull the
// owner in rather than emitting a second definition. atlcomcli.h includes only
// atldef.h, so there is no cycle back into this file.
#include <atlcomcli.h>

struct THUMBBUTTON;
struct _ICONINFO;
struct _NOTIFYICONDATAW;
struct _OVERLAPPED;
struct _PROPSHEETPAGEW;
struct _SHELLEXECUTEINFOW;
struct _SHFILEINFOW;
struct _WSABUF;
struct _charrange;
struct _enlink;
struct tagBITMAP;
struct tagLOGFONTW;
struct tagNCCALCSIZE_PARAMS;
struct tagNMCUSTOMDRAWINFO;
struct tagNMHDR;
struct tagNMLISTVIEW;
struct tagNMLVCUSTOMDRAW;
struct tagNMTREEVIEWW;
struct tagNMTTCUSTOMDRAW;
struct tagNONCLIENTMETRICSW;
struct tagPALETTEENTRY;
struct tagTVDISPINFOW;
struct tagTVITEMW;
struct tagWINDOWPOS;
struct tagWNDCLASSEXW;

typedef u_short ADDRESS_FAMILY;

typedef struct tagBITMAP {
    LONG bmType;
    LONG bmWidth;
    LONG bmHeight;
    LONG bmWidthBytes;
    WORD bmPlanes;
    WORD bmBitsPixel;
    LPVOID bmBits;
  } BITMAP,*PBITMAP,*NPBITMAP,*LPBITMAP;

#pragma pack(push, 4)
typedef struct _charrange {
    LONG cpMin;
    LONG cpMax;
  } CHARRANGE;
#pragma pack(pop)

typedef struct tagNMHDR {
    HWND hwndFrom;
    UINT_PTR idFrom;
    UINT code;
  } NMHDR;

#pragma pack(push, 4)
typedef struct _enlink {
    NMHDR nmhdr;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    CHARRANGE chrg;
  } ENLINK;
#pragma pack(pop)

typedef INT_PTR (*FARPROC) ();

typedef LPVOID HINTERNET;

typedef struct _PSP *HPROPSHEETPAGE;

typedef int WINBOOL;

typedef struct _ICONINFO {
    WINBOOL fIcon;
    DWORD xHotspot;
    DWORD yHotspot;
    HBITMAP hbmMask;
    HBITMAP hbmColor;
  } ICONINFO;

typedef struct IDropTargetHelper IDropTargetHelper;

typedef struct IEnumString IEnumString;

typedef struct IInternetSecurityMgrSite IInternetSecurityMgrSite;

typedef int INT;

typedef struct IPicture IPicture;

typedef struct IStorage IStorage;

typedef struct ITaskbarList3 ITaskbarList3;

typedef union _LARGE_INTEGER {
    struct {
      DWORD LowPart;
      LONG HighPart;
    } ;
    struct {
      DWORD LowPart;
      LONG HighPart;
    } u;
    LONGLONG QuadPart;
  } LARGE_INTEGER;

typedef long long LONG64,*PLONG64;

typedef BYTE *LPBYTE;

typedef DWORD *LPCOLORREF;

typedef const void *LPCVOID;

typedef int *LPINT;

typedef wchar_t WCHAR;

typedef struct tagLOGFONTW {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    WCHAR lfFaceName[32];
  } LOGFONTW,*PLOGFONTW,*NPLOGFONTW,*LPLOGFONTW;

typedef LPLOGFONTW LPLOGFONT;

typedef struct tagWINDOWPOS {
    HWND hwnd;
    HWND hwndInsertAfter;
    int x;
    int y;
    int cx;
    int cy;
    UINT flags;
  } WINDOWPOS,*LPWINDOWPOS,*PWINDOWPOS;

typedef struct tagNCCALCSIZE_PARAMS {
    RECT rgrc[3];
    PWINDOWPOS lppos;
  } NCCALCSIZE_PARAMS,*LPNCCALCSIZE_PARAMS;

typedef NMHDR *LPNMHDR;

typedef struct tagNMLISTVIEW {
    NMHDR hdr;
    int iItem;
    int iSubItem;
    UINT uNewState;
    UINT uOldState;
    UINT uChanged;
    POINT ptAction;
    LPARAM lParam;
  } NMLISTVIEW,*LPNMLISTVIEW;

typedef struct _TREEITEM *HTREEITEM;

typedef struct tagTVITEMW {
    UINT mask;
    HTREEITEM hItem;
    UINT state;
    UINT stateMask;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int cChildren;
    LPARAM lParam;
  } TVITEMW,*LPTVITEMW;

typedef struct tagNMTREEVIEWW {
    NMHDR hdr;
    UINT action;
    TVITEMW itemOld;
    TVITEMW itemNew;
    POINT ptDrag;
  } NMTREEVIEWW,*LPNMTREEVIEWW;

typedef struct tagNMCUSTOMDRAWINFO {
    NMHDR hdr;
    DWORD dwDrawStage;
    HDC hdc;
    RECT rc;
    DWORD_PTR dwItemSpec;
    UINT uItemState;
    LPARAM lItemlParam;
  } NMCUSTOMDRAW,*LPNMCUSTOMDRAW;

typedef struct tagNMTTCUSTOMDRAW {
    NMCUSTOMDRAW nmcd;
    UINT uDrawFlags;
  } NMTTCUSTOMDRAW,*LPNMTTCUSTOMDRAW;

typedef struct tagTVDISPINFOW {
    NMHDR hdr;
    TVITEMW item;
  } NMTVDISPINFOW,*LPNMTVDISPINFOW;

#pragma pack(push, 2)
typedef struct {
    DWORD style;
    DWORD dwExtendedStyle;
    WORD cdit;
    short x;
    short y;
    short cx;
    short cy;
  } DLGTEMPLATE;
#pragma pack(pop)

typedef const DLGTEMPLATE *LPCDLGTEMPLATEW;

typedef LPCDLGTEMPLATEW LPCDLGTEMPLATE;

typedef LPCDLGTEMPLATE PROPSHEETPAGE_RESOURCE;

typedef UINT (*LPFNPSPCALLBACKW)(HWND hwnd,UINT uMsg,struct _PROPSHEETPAGEW *ppsp);

typedef INT_PTR (*DLGPROC) (HWND, UINT, WPARAM, LPARAM);

#pragma pack(push, 8)
typedef struct _PROPSHEETPAGEW {
    DWORD dwSize, dwFlags; HINSTANCE hInstance; union { LPCWSTR pszTemplate; PROPSHEETPAGE_RESOURCE pResource; } ; union { HICON hIcon; LPCWSTR pszIcon; } ; LPCWSTR pszTitle; DLGPROC pfnDlgProc; LPARAM lParam; LPFNPSPCALLBACKW pfnCallback; UINT *pcRefParent;
      LPCWSTR pszHeaderTitle;
    LPCWSTR pszHeaderSubTitle;
    HANDLE hActCtx;
  } PROPSHEETPAGEW_V3,*LPPROPSHEETPAGEW_V3;
#pragma pack(pop)

typedef LPPROPSHEETPAGEW_V3 LPPROPSHEETPAGEW;

typedef void *PVOID;

typedef struct _OVERLAPPED {
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union {
        struct {
            DWORD Offset;
            DWORD OffsetHigh;
        } ;
        PVOID Pointer;
    } ;
    HANDLE hEvent;
  } OVERLAPPED, *LPOVERLAPPED;

typedef struct _OVERLAPPED *LPWSAOVERLAPPED;

typedef struct tagNMLVCUSTOMDRAW {
    NMCUSTOMDRAW nmcd;
    COLORREF clrText;
    COLORREF clrTextBk;
    int iSubItem;
    DWORD dwItemType;
    COLORREF clrFace;
    int iIconEffect;
    int iIconPhase;
    int iPartId;
    int iStateId;
    RECT rcText;
    UINT uAlign;
  } NMLVCUSTOMDRAW,*LPNMLVCUSTOMDRAW;

typedef struct tagNONCLIENTMETRICSW {
    UINT cbSize;
    int iBorderWidth;
    int iScrollWidth;
    int iScrollHeight;
    int iCaptionWidth;
    int iCaptionHeight;
    LOGFONTW lfCaptionFont;
    int iSmCaptionWidth;
    int iSmCaptionHeight;
    LOGFONTW lfSmCaptionFont;
    int iMenuWidth;
    int iMenuHeight;
    LOGFONTW lfMenuFont;
    LOGFONTW lfStatusFont;
    LOGFONTW lfMessageFont;
    int iPaddedBorderWidth;
  } NONCLIENTMETRICSW,*PNONCLIENTMETRICSW,*LPNONCLIENTMETRICSW;

typedef NONCLIENTMETRICSW NONCLIENTMETRICS;

typedef struct _NOTIFYICONDATAW {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    WCHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    WCHAR szInfo[256];
    union {
      UINT uTimeout;
      UINT uVersion;
    } ;
    WCHAR szInfoTitle[64];
    DWORD dwInfoFlags;
    GUID guidItem;
    HICON hBalloonIcon;
  } NOTIFYICONDATAW,*PNOTIFYICONDATAW;

typedef NOTIFYICONDATAW NOTIFYICONDATA;

typedef struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
  } PALETTEENTRY,*PPALETTEENTRY,*LPPALETTEENTRY;

typedef int (*PFNLVCOMPARE)(LPARAM,LPARAM,LPARAM);

typedef struct _SHELLEXECUTEINFOW {
    DWORD cbSize;
    ULONG fMask;
    HWND hwnd;
    LPCWSTR lpVerb;
    LPCWSTR lpFile;
    LPCWSTR lpParameters;
    LPCWSTR lpDirectory;
    int nShow;
    HINSTANCE hInstApp;
    void *lpIDList;
    LPCWSTR lpClass;
    HKEY hkeyClass;
    DWORD dwHotKey;
    union {
      HANDLE hIcon;
      HANDLE hMonitor;
    } ;
    HANDLE hProcess;
  } SHELLEXECUTEINFOW,*LPSHELLEXECUTEINFOW;

typedef SHELLEXECUTEINFOW SHELLEXECUTEINFO;

typedef struct _SHFILEINFOW {
    HICON hIcon;
    int iIcon;
    DWORD dwAttributes;
    WCHAR szDisplayName[260];
    WCHAR szTypeName[80];
  } SHFILEINFOW;

typedef SHFILEINFOW SHFILEINFO;

typedef enum TBPFLAG {
    TBPF_NOPROGRESS = 0x0,
    TBPF_INDETERMINATE = 0x1,
    TBPF_NORMAL = 0x2,
    TBPF_ERROR = 0x4,
    TBPF_PAUSED = 0x8
} TBPFLAG;

typedef enum THUMBBUTTONMASK {
    THB_BITMAP = 0x1,
    THB_ICON = 0x2,
    THB_TOOLTIP = 0x4,
    THB_FLAGS = 0x8
} THUMBBUTTONMASK;

typedef enum THUMBBUTTONFLAGS {
    THBF_ENABLED = 0x0,
    THBF_DISABLED = 0x1,
    THBF_DISMISSONCLICK = 0x2,
    THBF_NOBACKGROUND = 0x4,
    THBF_HIDDEN = 0x8,
    THBF_NONINTERACTIVE = 0x10
} THUMBBUTTONFLAGS;

#pragma pack(push, 8)
typedef struct THUMBBUTTON {
    THUMBBUTTONMASK dwMask;
    UINT iId;
    UINT iBitmap;
    HICON hIcon;
    WCHAR szTip[260];
    THUMBBUTTONFLAGS dwFlags;
} THUMBBUTTON;
#pragma pack(pop)

typedef unsigned short UINT16,*PUINT16;

typedef unsigned int UINT32,*PUINT32;

typedef unsigned long long UINT64,*PUINT64;

#pragma pack(push, 8)
typedef enum tagURLZONE {
    URLZONE_INVALID = -1,
    URLZONE_PREDEFINED_MIN = 0,
    URLZONE_LOCAL_MACHINE = 0,
    URLZONE_INTRANET = 1,
    URLZONE_TRUSTED = 2,
    URLZONE_INTERNET = 3,
    URLZONE_UNTRUSTED = 4,
    URLZONE_PREDEFINED_MAX = 999,
    URLZONE_USER_MIN = 1000,
    URLZONE_USER_MAX = 10000
} URLZONE;
#pragma pack(pop)

typedef LRESULT (*WNDPROC)(HWND,UINT,WPARAM,LPARAM);

typedef struct tagWNDCLASSEXW {
    UINT cbSize;
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCWSTR lpszMenuName;
    LPCWSTR lpszClassName;
    HICON hIconSm;
  } WNDCLASSEXW,*PWNDCLASSEXW,*NPWNDCLASSEXW,*LPWNDCLASSEXW;

typedef WNDCLASSEXW WNDCLASSEX;

typedef struct _WSABUF {
    u_long len;
    char *buf;
  } WSABUF,*LPWSABUF;

typedef unsigned char boolean;

