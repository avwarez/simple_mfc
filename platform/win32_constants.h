// win32_constants.h -- GENERATED, do not edit by hand.
//
// Every value here was computed by clang preprocessing the mingw-w64 headers
// with a Windows target; none of it was transcribed by hand. Regenerate with
// scratchpad/mkconst.py rather than patching a value in place.
//
// Only the constants eMule actually failed on are emitted, plus whatever they
// transitively reference -- this is not a copy of the SDK.
//
// Guarded individually because afx.h/atltypes.h own a few of these names and
// must keep owning them: single owner per symbol.

#pragma once

#ifndef ABE_BOTTOM
#define ABE_BOTTOM 3
#endif
#ifndef ABE_LEFT
#define ABE_LEFT 0
#endif
#ifndef ABE_RIGHT
#define ABE_RIGHT 2
#endif
#ifndef ABE_TOP
#define ABE_TOP 1
#endif
#ifndef ANSI_CHARSET
#define ANSI_CHARSET 0
#endif
#ifndef ANTIALIASED_QUALITY
#define ANTIALIASED_QUALITY 4
#endif
#ifndef BDR_SUNKENINNER
#define BDR_SUNKENINNER 0x0008
#endif
#ifndef BF_LEFT
#define BF_LEFT 0x0001
#endif
#ifndef BF_TOP
#define BF_TOP 0x0002
#endif
#ifndef BF_RIGHT
#define BF_RIGHT 0x0004
#endif
#ifndef BF_BOTTOM
#define BF_BOTTOM 0x0008
#endif
#ifndef BF_RECT
#define BF_RECT (BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM)
#endif
#ifndef BLACKNESS
#define BLACKNESS (DWORD)0x00000042
#endif
#ifndef BLACK_BRUSH
#define BLACK_BRUSH 4
#endif
#ifndef __MSABI_LONG
#define __MSABI_LONG(x) x ## l
#endif
#ifndef BS_FLAT
#define BS_FLAT 0x00008000
#endif
#ifndef BS_OWNERDRAW
#define BS_OWNERDRAW 0x0000000B
#endif
#ifndef TBSTYLE_AUTOSIZE
#define TBSTYLE_AUTOSIZE 0x10
#endif
#ifndef BTNS_AUTOSIZE
#define BTNS_AUTOSIZE TBSTYLE_AUTOSIZE
#endif
#ifndef TBSTYLE_BUTTON
#define TBSTYLE_BUTTON 0x0
#endif
#ifndef BTNS_BUTTON
#define BTNS_BUTTON TBSTYLE_BUTTON
#endif
#ifndef TBSTYLE_DROPDOWN
#define TBSTYLE_DROPDOWN 0x8
#endif
#ifndef BTNS_DROPDOWN
#define BTNS_DROPDOWN TBSTYLE_DROPDOWN
#endif
#ifndef TBSTYLE_SEP
#define TBSTYLE_SEP 0x1
#endif
#ifndef BTNS_SEP
#define BTNS_SEP TBSTYLE_SEP
#endif
#ifndef BTNS_WHOLEDROPDOWN
#define BTNS_WHOLEDROPDOWN 0x80
#endif
#ifndef CBEIF_IMAGE
#define CBEIF_IMAGE 0x2
#endif
#ifndef CBEIF_SELECTEDIMAGE
#define CBEIF_SELECTEDIMAGE 0x4
#endif
#ifndef CBEIF_TEXT
#define CBEIF_TEXT 0x1
#endif
#ifndef CBS_DROPDOWN
#define CBS_DROPDOWN 0x0002
#endif
#ifndef CB_ERR
#define CB_ERR (-1)
#endif
#ifndef CB_SETDROPPEDWIDTH
#define CB_SETDROPPEDWIDTH 0x0160
#endif
#ifndef CCM_FIRST
#define CCM_FIRST 0x2000
#endif
#ifndef CCM_SETUNICODEFORMAT
#define CCM_SETUNICODEFORMAT (CCM_FIRST+5)
#endif
#ifndef CCS_NODIVIDER
#define CCS_NODIVIDER 0x40
#endif
#ifndef CCS_NOMOVEY
#define CCS_NOMOVEY 0x2
#endif
#ifndef CCS_NOPARENTALIGN
#define CCS_NOPARENTALIGN 0x8
#endif
#ifndef CCS_NORESIZE
#define CCS_NORESIZE 0x4
#endif
#ifndef CDDS_ITEM
#define CDDS_ITEM 0x10000
#endif
#ifndef CDDS_PREPAINT
#define CDDS_PREPAINT 0x1
#endif
#ifndef CDDS_ITEMPREPAINT
#define CDDS_ITEMPREPAINT (CDDS_ITEM | CDDS_PREPAINT)
#endif
#ifndef CDRF_DODEFAULT
#define CDRF_DODEFAULT 0x0
#endif
#ifndef CDRF_NOTIFYITEMDRAW
#define CDRF_NOTIFYITEMDRAW 0x20
#endif
#ifndef CFE_AUTOCOLOR
#define CFE_AUTOCOLOR 0x40000000
#endif
#ifndef CFE_BOLD
#define CFE_BOLD 0x00000001
#endif
#ifndef CFM_BOLD
#define CFM_BOLD 0x00000001
#endif
#ifndef CFM_COLOR
#define CFM_COLOR 0x40000000
#endif
#ifndef CLIP_DEFAULT_PRECIS
#define CLIP_DEFAULT_PRECIS 0
#endif
#ifndef CLR_DEFAULT
#define CLR_DEFAULT 0xff000000
#endif
#ifndef CLR_INVALID
#define CLR_INVALID 0xFFFFFFFF
#endif
#ifndef CLR_NONE
#define CLR_NONE 0xffffffff
#endif
#ifndef COLOR_BTNFACE
#define COLOR_BTNFACE 15
#endif
#ifndef COLOR_3DFACE
#define COLOR_3DFACE COLOR_BTNFACE
#endif
#ifndef CONST
#define CONST const
#endif
#ifndef CS_CLASSDC
#define CS_CLASSDC 0x0040
#endif
#ifndef CS_DBLCLKS
#define CS_DBLCLKS 0x0008
#endif
#ifndef CS_HREDRAW
#define CS_HREDRAW 0x0002
#endif
#ifndef CS_SAVEBITS
#define CS_SAVEBITS 0x0800
#endif
#ifndef CS_VREDRAW
#define CS_VREDRAW 0x0001
#endif
#ifndef CWP_SKIPDISABLED
#define CWP_SKIPDISABLED 0x0002
#endif
#ifndef CWP_SKIPINVISIBLE
#define CWP_SKIPINVISIBLE 0x0001
#endif
#ifndef __MINGW_NAME_AW
#define __MINGW_NAME_AW(func) func##W
#endif
#ifndef CallWindowProc
#define CallWindowProc __MINGW_NAME_AW(CallWindowProc)
#endif
#ifndef CopyFile
#define CopyFile __MINGW_NAME_AW(CopyFile)
#endif
#ifndef CreateEvent
#define CreateEvent __MINGW_NAME_AW(CreateEvent)
#endif
#ifndef CreateFile
#define CreateFile __MINGW_NAME_AW(CreateFile)
#endif
#ifndef CreatePropertySheetPage
#define CreatePropertySheetPage __MINGW_NAME_AW(CreatePropertySheetPage)
#endif
#ifndef DEFAULT_CHARSET
#define DEFAULT_CHARSET 1
#endif
#ifndef DEFAULT_PITCH
#define DEFAULT_PITCH 0
#endif
#ifndef DEFAULT_QUALITY
#define DEFAULT_QUALITY 0
#endif
#ifndef DFCS_ADJUSTRECT
#define DFCS_ADJUSTRECT 0x2000
#endif
#ifndef DFCS_BUTTONPUSH
#define DFCS_BUTTONPUSH 0x0010
#endif
#ifndef DFCS_INACTIVE
#define DFCS_INACTIVE 0x0100
#endif
#ifndef DFCS_PUSHED
#define DFCS_PUSHED 0x0200
#endif
#ifndef DFC_BUTTON
#define DFC_BUTTON 4
#endif
#ifndef DI_IMAGE
#define DI_IMAGE 0x0002
#endif
#ifndef DI_MASK
#define DI_MASK 0x0001
#endif
#ifndef DI_NORMAL
#define DI_NORMAL 0x0003
#endif
#ifndef DLGC_HASSETSEL
#define DLGC_HASSETSEL 0x0008
#endif
#ifndef DLGC_WANTALLKEYS
#define DLGC_WANTALLKEYS 0x0004
#endif
#ifndef DLGC_WANTTAB
#define DLGC_WANTTAB 0x0002
#endif
#ifndef DSS_DISABLED
#define DSS_DISABLED 0x0020
#endif
#ifndef DSS_NORMAL
#define DSS_NORMAL 0x0000
#endif
#ifndef DST_ICON
#define DST_ICON 0x0003
#endif
#ifndef DST_TEXT
#define DST_TEXT 0x0001
#endif
#ifndef DT_BOTTOM
#define DT_BOTTOM 0x00000008
#endif
#ifndef DT_CALCRECT
#define DT_CALCRECT 0x00000400
#endif
#ifndef DT_CENTER
#define DT_CENTER 0x00000001
#endif
#ifndef DT_END_ELLIPSIS
#define DT_END_ELLIPSIS 0x00008000
#endif
#ifndef DT_EXPANDTABS
#define DT_EXPANDTABS 0x00000040
#endif
#ifndef DT_LEFT
#define DT_LEFT 0x00000000
#endif
#ifndef DT_NOCLIP
#define DT_NOCLIP 0x00000100
#endif
#ifndef DT_NOPREFIX
#define DT_NOPREFIX 0x00000800
#endif
#ifndef DT_PATH_ELLIPSIS
#define DT_PATH_ELLIPSIS 0x00004000
#endif
#ifndef DT_RIGHT
#define DT_RIGHT 0x00000002
#endif
#ifndef DT_SINGLELINE
#define DT_SINGLELINE 0x00000020
#endif
#ifndef DT_TOP
#define DT_TOP 0x00000000
#endif
#ifndef DT_VCENTER
#define DT_VCENTER 0x00000004
#endif
#ifndef DT_WORDBREAK
#define DT_WORDBREAK 0x00000010
#endif
#ifndef DefWindowProc
#define DefWindowProc __MINGW_NAME_AW(DefWindowProc)
#endif
#ifndef DeleteFile
#define DeleteFile __MINGW_NAME_AW(DeleteFile)
#endif
#ifndef DrawText
#define DrawText __MINGW_NAME_AW(DrawText)
#endif
#ifndef BDR_SUNKENOUTER
#define BDR_SUNKENOUTER 0x0002
#endif
#ifndef BDR_RAISEDINNER
#define BDR_RAISEDINNER 0x0004
#endif
#ifndef EDGE_ETCHED
#define EDGE_ETCHED (BDR_SUNKENOUTER | BDR_RAISEDINNER)
#endif
#ifndef BDR_RAISEDOUTER
#define BDR_RAISEDOUTER 0x0001
#endif
#ifndef EDGE_RAISED
#define EDGE_RAISED (BDR_RAISEDOUTER | BDR_RAISEDINNER)
#endif
#ifndef EDGE_SUNKEN
#define EDGE_SUNKEN (BDR_SUNKENOUTER | BDR_SUNKENINNER)
#endif
#ifndef EM_SETREADONLY
#define EM_SETREADONLY 0x00CF
#endif
#ifndef ENM_CHANGE
#define ENM_CHANGE 0x00000001
#endif
#ifndef EN_LINK
#define EN_LINK 0x070b
#endif
#ifndef EN_REQUESTRESIZE
#define EN_REQUESTRESIZE 0x0701
#endif
#ifndef ERROR_INVALID_PARAMETER
#define ERROR_INVALID_PARAMETER 87
#endif
#ifndef ERROR_IO_PENDING
#define ERROR_IO_PENDING 997
#endif
#ifndef ES_AUTOHSCROLL
#define ES_AUTOHSCROLL 0x0080
#endif
#ifndef ES_LEFT
#define ES_LEFT 0x0000
#endif
#ifndef ES_NUMBER
#define ES_NUMBER 0x2000
#endif
#ifndef ExtTextOut
#define ExtTextOut __MINGW_NAME_AW(ExtTextOut)
#endif
#ifndef FF_MODERN
#define FF_MODERN (3<<4)
#endif
#ifndef FF_SWISS
#define FF_SWISS (2<<4)
#endif
#ifndef FILE_ATTRIBUTE_COMPRESSED
#define FILE_ATTRIBUTE_COMPRESSED 0x00000800
#endif
#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#endif
#ifndef FILE_ATTRIBUTE_SPARSE_FILE
#define FILE_ATTRIBUTE_SPARSE_FILE 0x00000200
#endif
#ifndef FILE_FLAG_OVERLAPPED
#define FILE_FLAG_OVERLAPPED 0x40000000
#endif
#ifndef FILE_FLAG_SEQUENTIAL_SCAN
#define FILE_FLAG_SEQUENTIAL_SCAN 0x8000000
#endif
#ifndef FILE_SHARE_DELETE
#define FILE_SHARE_DELETE 0x00000004
#endif
#ifndef FILE_SHARE_READ
#define FILE_SHARE_READ 0x00000001
#endif
#ifndef FILE_SHARE_WRITE
#define FILE_SHARE_WRITE 0x00000002
#endif
#ifndef FIXED_PITCH
#define FIXED_PITCH 1
#endif
#ifndef FW_BOLD
#define FW_BOLD 700
#endif
#ifndef FW_HEAVY
#define FW_HEAVY 900
#endif
#ifndef FW_NORMAL
#define FW_NORMAL 400
#endif
#ifndef FindResource
#define FindResource __MINGW_NAME_AW(FindResource)
#endif
#ifndef FindWindow
#define FindWindow __MINGW_NAME_AW(FindWindow)
#endif
#ifndef GDI_ERROR
#define GDI_ERROR (0xFFFFFFFF)
#endif
#ifndef GENERIC_READ
#define GENERIC_READ (0x80000000)
#endif
#ifndef GENERIC_WRITE
#define GENERIC_WRITE (0x40000000)
#endif
#ifndef GMEM_MOVEABLE
#define GMEM_MOVEABLE 0x2
#endif
#ifndef GWLP_ID
#define GWLP_ID (-12)
#endif
#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC (-4)
#endif
#ifndef GWL_EXSTYLE
#define GWL_EXSTYLE (-20)
#endif
#ifndef GWL_STYLE
#define GWL_STYLE (-16)
#endif
#ifndef GW_CHILD
#define GW_CHILD 5
#endif
#ifndef GW_HWNDNEXT
#define GW_HWNDNEXT 2
#endif
#ifndef GW_HWNDPREV
#define GW_HWNDPREV 3
#endif
#ifndef GetClassName
#define GetClassName __MINGW_NAME_AW(GetClassName)
#endif
#ifndef GetCurrentDirectory
#define GetCurrentDirectory __MINGW_NAME_AW(GetCurrentDirectory)
#endif
#ifndef GetLogicalDriveStrings
#define GetLogicalDriveStrings GetLogicalDriveStringsW
#endif
#ifndef GetModuleFileName
#define GetModuleFileName __MINGW_NAME_AW(GetModuleFileName)
#endif
#ifndef GetModuleHandle
#define GetModuleHandle __MINGW_NAME_AW(GetModuleHandle)
#endif
#ifndef GetPrivateProfileString
#define GetPrivateProfileString __MINGW_NAME_AW(GetPrivateProfileString)
#endif
#ifndef GetProp
#define GetProp __MINGW_NAME_AW(GetProp)
#endif
#ifndef GetTextExtentPoint32
#define GetTextExtentPoint32 __MINGW_NAME_AW(GetTextExtentPoint32)
#endif
#ifndef GetWindowLongPtr
#define GetWindowLongPtr __MINGW_NAME_AW(GetWindowLongPtr)
#endif
#ifndef HDI_FORMAT
#define HDI_FORMAT 0x4
#endif
#ifndef HDI_IMAGE
#define HDI_IMAGE 0x20
#endif
#ifndef HDI_TEXT
#define HDI_TEXT 0x2
#endif
#ifndef HDI_WIDTH
#define HDI_WIDTH 0x1
#endif
#ifndef HDN_FIRST
#define HDN_FIRST (0U-300U)
#endif
#ifndef HDN_BEGINDRAG
#define HDN_BEGINDRAG (HDN_FIRST-10)
#endif
#ifndef HDN_BEGINTRACKA
#define HDN_BEGINTRACKA (HDN_FIRST-6)
#endif
#ifndef HDN_BEGINTRACKW
#define HDN_BEGINTRACKW (HDN_FIRST-26)
#endif
#ifndef HDN_ENDDRAG
#define HDN_ENDDRAG (HDN_FIRST-11)
#endif
#ifndef HTCLIENT
#define HTCLIENT 1
#endif
#ifndef HTNOWHERE
#define HTNOWHERE 0
#endif
#ifndef HTTRANSPARENT
#define HTTRANSPARENT (-1)
#endif
#ifndef HWND_DESKTOP
#define HWND_DESKTOP ((HWND)0)
#endif
#ifndef SendMessage
#define SendMessage __MINGW_NAME_AW(SendMessage)
#endif
#ifndef SNDMSG
#define SNDMSG ::SendMessage
#endif
#ifndef HDM_FIRST
#define HDM_FIRST 0x1200
#endif
#ifndef HDM_GETORDERARRAY
#define HDM_GETORDERARRAY (HDM_FIRST+17)
#endif
#ifndef Header_GetOrderArray
#define Header_GetOrderArray(hwnd,iCount,lpi) (WINBOOL)SNDMSG((hwnd),HDM_GETORDERARRAY,(WPARAM)(iCount),(LPARAM)(lpi))
#endif
#ifndef MAKEINTRESOURCE
#define MAKEINTRESOURCE __MINGW_NAME_AW(MAKEINTRESOURCE)
#endif
#ifndef IDC_ARROW
#define IDC_ARROW MAKEINTRESOURCE(32512)
#endif
#ifndef IDC_HAND
#define IDC_HAND MAKEINTRESOURCE(32649)
#endif
#ifndef IDC_SIZEALL
#define IDC_SIZEALL MAKEINTRESOURCE(32646)
#endif
#ifndef IDC_SIZENS
#define IDC_SIZENS MAKEINTRESOURCE(32645)
#endif
#ifndef IDC_SIZEWE
#define IDC_SIZEWE MAKEINTRESOURCE(32644)
#endif
#ifndef IDHELP
#define IDHELP 9
#endif
#ifndef IDI_QUESTION
#define IDI_QUESTION MAKEINTRESOURCE(32514)
#endif
#ifndef ILC_COLOR
#define ILC_COLOR 0x0
#endif
#ifndef ILC_MASK
#define ILC_MASK 0x1
#endif
#ifndef ILD_NORMAL
#define ILD_NORMAL 0x0
#endif
#ifndef InterlockedExchange
#define InterlockedExchange _InterlockedExchange
#endif
#ifndef InterlockedExchange64
#define InterlockedExchange64 _InterlockedExchange64
#endif
#ifndef LANG_ARABIC
#define LANG_ARABIC 0x01
#endif
#ifndef LANG_BASQUE
#define LANG_BASQUE 0x2d
#endif
#ifndef LANG_BRETON
#define LANG_BRETON 0x7e
#endif
#ifndef LANG_BULGARIAN
#define LANG_BULGARIAN 0x02
#endif
#ifndef LANG_CATALAN
#define LANG_CATALAN 0x03
#endif
#ifndef LANG_CZECH
#define LANG_CZECH 0x05
#endif
#ifndef LANG_DANISH
#define LANG_DANISH 0x06
#endif
#ifndef LANG_DUTCH
#define LANG_DUTCH 0x13
#endif
#ifndef LANG_ESTONIAN
#define LANG_ESTONIAN 0x25
#endif
#ifndef LANG_FARSI
#define LANG_FARSI 0x29
#endif
#ifndef LANG_FINNISH
#define LANG_FINNISH 0x0b
#endif
#ifndef LANG_FRENCH
#define LANG_FRENCH 0x0c
#endif
#ifndef LANG_GALICIAN
#define LANG_GALICIAN 0x56
#endif
#ifndef LANG_GERMAN
#define LANG_GERMAN 0x07
#endif
#ifndef LANG_GREEK
#define LANG_GREEK 0x08
#endif
#ifndef LANG_HEBREW
#define LANG_HEBREW 0x0d
#endif
#ifndef LANG_HUNGARIAN
#define LANG_HUNGARIAN 0x0e
#endif
#ifndef LANG_ITALIAN
#define LANG_ITALIAN 0x10
#endif
#ifndef LANG_JAPANESE
#define LANG_JAPANESE 0x11
#endif
#ifndef LANG_KOREAN
#define LANG_KOREAN 0x12
#endif
#ifndef LANG_LATVIAN
#define LANG_LATVIAN 0x26
#endif
#ifndef LANG_LITHUANIAN
#define LANG_LITHUANIAN 0x27
#endif
#ifndef LANG_MALTESE
#define LANG_MALTESE 0x3a
#endif
#ifndef LANG_NORWEGIAN
#define LANG_NORWEGIAN 0x14
#endif
#ifndef LANG_SPANISH
#define LANG_SPANISH 0x0a
#endif
#ifndef LBN_DBLCLK
#define LBN_DBLCLK 2
#endif
#ifndef LB_ERR
#define LB_ERR (-1)
#endif
#ifndef LB_ERRSPACE
#define LB_ERRSPACE (-2)
#endif
#ifndef LF_FACESIZE
#define LF_FACESIZE 32
#endif
#ifndef LOGPIXELSX
#define LOGPIXELSX 88
#endif
#ifndef LOGPIXELSY
#define LOGPIXELSY 90
#endif
#ifndef LPNMTREEVIEW
#define LPNMTREEVIEW __MINGW_NAME_AW(LPNMTREEVIEW)
#endif
#ifndef LPNMTVDISPINFO
#define LPNMTVDISPINFO __MINGW_NAME_AW(LPNMTVDISPINFO)
#endif
#ifndef LPPROPSHEETPAGE
#define LPPROPSHEETPAGE __MINGW_NAME_AW(LPPROPSHEETPAGE)
#endif
#ifndef LVCFMT_CENTER
#define LVCFMT_CENTER 0x2
#endif
#ifndef LVCFMT_JUSTIFYMASK
#define LVCFMT_JUSTIFYMASK 0x3
#endif
#ifndef LVCFMT_RIGHT
#define LVCFMT_RIGHT 0x1
#endif
#ifndef LVCF_FMT
#define LVCF_FMT 0x1
#endif
#ifndef LVCF_ORDER
#define LVCF_ORDER 0x20
#endif
#ifndef LVCF_SUBITEM
#define LVCF_SUBITEM 0x8
#endif
#ifndef LVCF_TEXT
#define LVCF_TEXT 0x4
#endif
#ifndef LVCF_WIDTH
#define LVCF_WIDTH 0x2
#endif
#ifndef LVFI_PARAM
#define LVFI_PARAM 0x1
#endif
#ifndef LVIF_IMAGE
#define LVIF_IMAGE 0x2
#endif
#ifndef LVIF_PARAM
#define LVIF_PARAM 0x4
#endif
#ifndef LVIF_TEXT
#define LVIF_TEXT 0x1
#endif
#ifndef LVIR_LABEL
#define LVIR_LABEL 2
#endif
#ifndef LVIS_FOCUSED
#define LVIS_FOCUSED 0x1
#endif
#ifndef LVIS_SELECTED
#define LVIS_SELECTED 0x2
#endif
#ifndef LVM_FIRST
#define LVM_FIRST 0x1000
#endif
#ifndef LVM_SETIMAGELIST
#define LVM_SETIMAGELIST (LVM_FIRST+3)
#endif
#ifndef LVN_FIRST
#define LVN_FIRST (0U-100U)
#endif
#ifndef LVN_BEGINSCROLL
#define LVN_BEGINSCROLL (LVN_FIRST-80)
#endif
#ifndef LVN_COLUMNCLICK
#define LVN_COLUMNCLICK (LVN_FIRST-8)
#endif
#ifndef LVN_DELETEALLITEMS
#define LVN_DELETEALLITEMS (LVN_FIRST-4)
#endif
#ifndef LVN_DELETEITEM
#define LVN_DELETEITEM (LVN_FIRST-3)
#endif
#ifndef LVN_ENDSCROLL
#define LVN_ENDSCROLL (LVN_FIRST-81)
#endif
#ifndef LVN_GETDISPINFO
#define LVN_GETDISPINFO __MINGW_NAME_AW(LVN_GETDISPINFO)
#endif
#ifndef LVN_GETDISPINFOW
#define LVN_GETDISPINFOW (LVN_FIRST-77)
#endif
#ifndef LVN_GETINFOTIP
#define LVN_GETINFOTIP __MINGW_NAME_AW(LVN_GETINFOTIP)
#endif
#ifndef LVN_GETINFOTIPW
#define LVN_GETINFOTIPW (LVN_FIRST-58)
#endif
#ifndef LVN_ITEMACTIVATE
#define LVN_ITEMACTIVATE (LVN_FIRST-14)
#endif
#ifndef LVSCW_AUTOSIZE
#define LVSCW_AUTOSIZE -1
#endif
#ifndef LVSIL_SMALL
#define LVSIL_SMALL 1
#endif
#ifndef LVS_EX_FULLROWSELECT
#define LVS_EX_FULLROWSELECT 0x20
#endif
#ifndef LVS_EX_HEADERDRAGDROP
#define LVS_EX_HEADERDRAGDROP 0x10
#endif
#ifndef LVS_EX_INFOTIP
#define LVS_EX_INFOTIP 0x400
#endif
#ifndef LoadAccelerators
#define LoadAccelerators __MINGW_NAME_AW(LoadAccelerators)
#endif
#ifndef LoadCursor
#define LoadCursor __MINGW_NAME_AW(LoadCursor)
#endif
#ifndef LoadIcon
#define LoadIcon __MINGW_NAME_AW(LoadIcon)
#endif
#ifndef LoadLibrary
#define LoadLibrary __MINGW_NAME_AW(LoadLibrary)
#endif
#ifndef LoadString
#define LoadString __MINGW_NAME_AW(LoadString)
#endif
#ifndef MAKEINTRESOURCEW
#define MAKEINTRESOURCEW(i) ((LPWSTR)((ULONG_PTR)((WORD)(i))))
#endif
#ifndef MAKELONG
#define MAKELONG(a,b) ((LONG) (((WORD) (((DWORD_PTR) (a)) & 0xffff)) | ((DWORD) ((WORD) (((DWORD_PTR) (b)) & 0xffff))) << 16))
#endif
#ifndef MAKELPARAM
#define MAKELPARAM(l,h) ((LPARAM)(DWORD)MAKELONG(l,h))
#endif
#ifndef MAKEWPARAM
#define MAKEWPARAM(l,h) ((WPARAM)(DWORD)MAKELONG(l,h))
#endif
#ifndef MAPVK_VK_TO_CHAR
#define MAPVK_VK_TO_CHAR (2)
#endif
#ifndef MAXGETHOSTSTRUCT
#define MAXGETHOSTSTRUCT 1024
#endif
#ifndef MIM_STYLE
#define MIM_STYLE 0x00000010
#endif
#ifndef MK_CONTROL
#define MK_CONTROL 0x0008
#endif
#ifndef MK_LBUTTON
#define MK_LBUTTON 0x0001
#endif
#ifndef MNS_CHECKORBMP
#define MNS_CHECKORBMP 0x04000000
#endif
#ifndef MapVirtualKey
#define MapVirtualKey __MINGW_NAME_AW(MapVirtualKey)
#endif
#ifndef NIF_ICON
#define NIF_ICON 0x00000002
#endif
#ifndef NIF_MESSAGE
#define NIF_MESSAGE 0x00000001
#endif
#ifndef NIF_TIP
#define NIF_TIP 0x00000004
#endif
#ifndef NIM_ADD
#define NIM_ADD 0x00000000
#endif
#ifndef NIM_DELETE
#define NIM_DELETE 0x00000002
#endif
#ifndef NIM_MODIFY
#define NIM_MODIFY 0x00000001
#endif
#ifndef NM_FIRST
#define NM_FIRST (0U- 0U)
#endif
#ifndef NM_CLICK
#define NM_CLICK (NM_FIRST-2)
#endif
#ifndef NM_CUSTOMDRAW
#define NM_CUSTOMDRAW (NM_FIRST-12)
#endif
#ifndef NM_DBLCLK
#define NM_DBLCLK (NM_FIRST-3)
#endif
#ifndef NONANTIALIASED_QUALITY
#define NONANTIALIASED_QUALITY 3
#endif
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(Type,Field) ((LONG) __builtin_offsetof(Type, Field))
#endif
#ifndef NOTIFYICONDATAW_V2_SIZE
#define NOTIFYICONDATAW_V2_SIZE FIELD_OFFSET (NOTIFYICONDATAW, guidItem)
#endif
#ifndef __MINGW_NAME_AW_EXT
#define __MINGW_NAME_AW_EXT(func,ext) func##W##ext
#endif
#ifndef NOTIFYICONDATA_V2_SIZE
#define NOTIFYICONDATA_V2_SIZE __MINGW_NAME_AW_EXT(NOTIFYICONDATA,_V2_SIZE)
#endif
#ifndef ODS_DISABLED
#define ODS_DISABLED 0x0004
#endif
#ifndef ODS_FOCUS
#define ODS_FOCUS 0x0010
#endif
#ifndef ODS_SELECTED
#define ODS_SELECTED 0x0001
#endif
#ifndef OFN_EXPLORER
#define OFN_EXPLORER 0x80000
#endif
#ifndef OFN_HIDEREADONLY
#define OFN_HIDEREADONLY 0x4
#endif
#ifndef OFN_OVERWRITEPROMPT
#define OFN_OVERWRITEPROMPT 0x2
#endif
#ifndef OPEN_EXISTING
#define OPEN_EXISTING 3
#endif
#ifndef OUT_DEFAULT_PRECIS
#define OUT_DEFAULT_PRECIS 0
#endif
#ifndef RGB
#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#endif
#ifndef PALETTERGB
#define PALETTERGB(r,g,b) (0x02000000 | RGB(r,g,b))
#endif
#ifndef WM_USER
#define WM_USER 0x0400
#endif
#ifndef PBM_DELTAPOS
#define PBM_DELTAPOS (WM_USER+3)
#endif
#ifndef PBM_SETBARCOLOR
#define PBM_SETBARCOLOR (WM_USER+9)
#endif
#ifndef CCM_SETBKCOLOR
#define CCM_SETBKCOLOR (CCM_FIRST+1)
#endif
#ifndef PBM_SETBKCOLOR
#define PBM_SETBKCOLOR CCM_SETBKCOLOR
#endif
#ifndef PBM_SETPOS
#define PBM_SETPOS (WM_USER+2)
#endif
#ifndef PBM_SETSTEP
#define PBM_SETSTEP (WM_USER+4)
#endif
#ifndef PBM_STEPIT
#define PBM_STEPIT (WM_USER+5)
#endif
#ifndef PBS_VERTICAL
#define PBS_VERTICAL 0x4
#endif
#ifndef PROOF_QUALITY
#define PROOF_QUALITY 2
#endif
#ifndef PSH_HASHELP
#define PSH_HASHELP 0x00000200
#endif
#ifndef PSM_ADDPAGE
#define PSM_ADDPAGE (WM_USER + 103)
#endif
#ifndef PSM_INSERTPAGE
#define PSM_INSERTPAGE (WM_USER + 119)
#endif
#ifndef PSM_ISDIALOGMESSAGE
#define PSM_ISDIALOGMESSAGE (WM_USER + 117)
#endif
#ifndef PSM_REMOVEPAGE
#define PSM_REMOVEPAGE (WM_USER + 102)
#endif
#ifndef PSM_SETCURSEL
#define PSM_SETCURSEL (WM_USER + 101)
#endif
#ifndef PSM_SETCURSELID
#define PSM_SETCURSELID (WM_USER + 114)
#endif
#ifndef PSP_HASHELP
#define PSP_HASHELP 0x00000020
#endif
#ifndef PSP_USEHICON
#define PSP_USEHICON 0x00000002
#endif
#ifndef PSP_USEICONID
#define PSP_USEICONID 0x00000004
#endif
#ifndef PSP_USETITLE
#define PSP_USETITLE 0x00000008
#endif
#ifndef PS_DOT
#define PS_DOT 2
#endif
#ifndef PS_SOLID
#define PS_SOLID 0
#endif
#ifndef PostMessage
#define PostMessage __MINGW_NAME_AW(PostMessage)
#endif
#ifndef R2_NOTXORPEN
#define R2_NOTXORPEN 10
#endif
#ifndef RegisterClassEx
#define RegisterClassEx __MINGW_NAME_AW(RegisterClassEx)
#endif
#ifndef RegisterWindowMessage
#define RegisterWindowMessage __MINGW_NAME_AW(RegisterWindowMessage)
#endif
#ifndef RemoveProp
#define RemoveProp __MINGW_NAME_AW(RemoveProp)
#endif
#ifndef SB_ENDSCROLL
#define SB_ENDSCROLL 8
#endif
#ifndef SB_LINEDOWN
#define SB_LINEDOWN 1
#endif
#ifndef SB_LINEUP
#define SB_LINEUP 0
#endif
#ifndef SB_TOP
#define SB_TOP 6
#endif
#ifndef SC_KEYMENU
#define SC_KEYMENU 0xF100
#endif
#ifndef SEE_MASK_NOCLOSEPROCESS
#define SEE_MASK_NOCLOSEPROCESS 0x40
#endif
#ifndef SF_RTF
#define SF_RTF 0x0002
#endif
#ifndef SHGFI_SMALLICON
#define SHGFI_SMALLICON 0x000000001
#endif
#ifndef SHGFI_SYSICONINDEX
#define SHGFI_SYSICONINDEX 0x000004000
#endif
#ifndef SHGetFileInfo
#define SHGetFileInfo __MINGW_NAME_AW(SHGetFileInfo)
#endif
#ifndef SM_CXEDGE
#define SM_CXEDGE 45
#endif
#ifndef SM_CXSCREEN
#define SM_CXSCREEN 0
#endif
#ifndef SM_CYEDGE
#define SM_CYEDGE 46
#endif
#ifndef SPI_GETNONCLIENTMETRICS
#define SPI_GETNONCLIENTMETRICS 0x0029
#endif
#ifndef SRCAND
#define SRCAND (DWORD)0x008800C6
#endif
#ifndef SRCCOPY
#define SRCCOPY (DWORD)0x00CC0020
#endif
#ifndef SRCINVERT
#define SRCINVERT (DWORD)0x00660046
#endif
#ifndef SRCPAINT
#define SRCPAINT (DWORD)0x00EE0086
#endif
#ifndef SS_NOTIFY
#define SS_NOTIFY 0x00000100
#endif
#ifndef __declspec
#define __declspec(a) __attribute__((a))
#endif
#ifndef DECLSPEC_NOTHROW
#define DECLSPEC_NOTHROW __declspec (nothrow)
#endif
#ifndef COM_DECLSPEC_NOTHROW
#define COM_DECLSPEC_NOTHROW DECLSPEC_NOTHROW
#endif
#ifndef __stdcall
#define __stdcall __attribute__((__stdcall__))
#endif
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE WINAPI
#endif
#ifndef STDMETHOD
#define STDMETHOD(method) virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE method
#endif
#ifndef STDMETHODIMP
#define STDMETHODIMP HRESULT WINAPI
#endif
#ifndef STDMETHODIMP_
#define STDMETHODIMP_(type) type WINAPI
#endif
#ifndef SUBLANG_ARABIC_UAE
#define SUBLANG_ARABIC_UAE 0x0e
#endif
#ifndef SUBLANG_NORWEGIAN_BOKMAL
#define SUBLANG_NORWEGIAN_BOKMAL 0x01
#endif
#ifndef SUBLANG_NORWEGIAN_NYNORSK
#define SUBLANG_NORWEGIAN_NYNORSK 0x02
#endif
#ifndef SUBLANG_SPANISH
#define SUBLANG_SPANISH 0x01
#endif
#ifndef SUCCEEDED
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#endif
#ifndef SWP_FRAMECHANGED
#define SWP_FRAMECHANGED 0x0020
#endif
#ifndef SWP_DRAWFRAME
#define SWP_DRAWFRAME SWP_FRAMECHANGED
#endif
#ifndef SWP_NOACTIVATE
#define SWP_NOACTIVATE 0x0010
#endif
#ifndef SWP_NOMOVE
#define SWP_NOMOVE 0x0002
#endif
#ifndef SWP_NOOWNERZORDER
#define SWP_NOOWNERZORDER 0x0200
#endif
#ifndef SWP_NOSIZE
#define SWP_NOSIZE 0x0001
#endif
#ifndef SWP_NOZORDER
#define SWP_NOZORDER 0x0004
#endif
#ifndef SWP_SHOWWINDOW
#define SWP_SHOWWINDOW 0x0040
#endif
#ifndef S_OK
#define S_OK ((HRESULT)0x00000000)
#endif
#ifndef SetCurrentDirectory
#define SetCurrentDirectory __MINGW_NAME_AW(SetCurrentDirectory)
#endif
#ifndef SetProp
#define SetProp __MINGW_NAME_AW(SetProp)
#endif
#ifndef SetWindowLongPtr
#define SetWindowLongPtr __MINGW_NAME_AW(SetWindowLongPtr)
#endif
#ifndef Shell_NotifyIcon
#define Shell_NotifyIcon __MINGW_NAME_AW(Shell_NotifyIcon)
#endif
#ifndef TA_BASELINE
#define TA_BASELINE 24
#endif
#ifndef TA_BOTTOM
#define TA_BOTTOM 8
#endif
#ifndef TA_CENTER
#define TA_CENTER 6
#endif
#ifndef TA_LEFT
#define TA_LEFT 0
#endif
#ifndef TA_RIGHT
#define TA_RIGHT 2
#endif
#ifndef TA_TOP
#define TA_TOP 0
#endif
#ifndef TBIF_IMAGE
#define TBIF_IMAGE 0x1
#endif
#ifndef TBIF_SIZE
#define TBIF_SIZE 0x40
#endif
#ifndef TBIF_STYLE
#define TBIF_STYLE 0x8
#endif
#ifndef TBIF_TEXT
#define TBIF_TEXT 0x2
#endif
#ifndef TBN_FIRST
#define TBN_FIRST (0U-700U)
#endif
#ifndef TBN_DROPDOWN
#define TBN_DROPDOWN (TBN_FIRST - 10)
#endif
#ifndef TBSTATE_ENABLED
#define TBSTATE_ENABLED 0x4
#endif
#ifndef TBSTATE_WRAP
#define TBSTATE_WRAP 0x20
#endif
#ifndef TBSTYLE_EX_DRAWDDARROWS
#define TBSTYLE_EX_DRAWDDARROWS 0x1
#endif
#ifndef TBSTYLE_FLAT
#define TBSTYLE_FLAT 0x800
#endif
#ifndef TBSTYLE_LIST
#define TBSTYLE_LIST 0x1000
#endif
#ifndef TBSTYLE_TRANSPARENT
#define TBSTYLE_TRANSPARENT 0x8000
#endif
#ifndef TB_GETPADDING
#define TB_GETPADDING (WM_USER+86)
#endif
#ifndef TB_SETPADDING
#define TB_SETPADDING (WM_USER+87)
#endif
#ifndef TCS_HOTTRACK
#define TCS_HOTTRACK 0x40
#endif
#ifndef TTDT_INITIAL
#define TTDT_INITIAL 3
#endif
#ifndef TVE_COLLAPSE
#define TVE_COLLAPSE 0x1
#endif
#ifndef TVE_EXPAND
#define TVE_EXPAND 0x2
#endif
#ifndef TVE_TOGGLE
#define TVE_TOGGLE 0x3
#endif
#ifndef TVGN_CARET
#define TVGN_CARET 0x9
#endif
#ifndef TVGN_NEXT
#define TVGN_NEXT 0x1
#endif
#ifndef TVHT_ONITEMICON
#define TVHT_ONITEMICON 0x2
#endif
#ifndef TVHT_ONITEMLABEL
#define TVHT_ONITEMLABEL 0x4
#endif
#ifndef TVHT_ONITEMSTATEICON
#define TVHT_ONITEMSTATEICON 0x40
#endif
#ifndef TVHT_ONITEM
#define TVHT_ONITEM (TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMSTATEICON)
#endif
#ifndef TVIF_HANDLE
#define TVIF_HANDLE 0x10
#endif
#ifndef TVIF_TEXT
#define TVIF_TEXT 0x1
#endif
#ifndef TVIS_BOLD
#define TVIS_BOLD 0x10
#endif
#ifndef TVIS_EXPANDED
#define TVIS_EXPANDED 0x20
#endif
#ifndef TVIS_SELECTED
#define TVIS_SELECTED 0x2
#endif
#ifndef TVN_BEGINDRAG
#define TVN_BEGINDRAG __MINGW_NAME_AW(TVN_BEGINDRAG)
#endif
#ifndef TVN_FIRST
#define TVN_FIRST (0U-400U)
#endif
#ifndef TVN_BEGINDRAGW
#define TVN_BEGINDRAGW (TVN_FIRST-56)
#endif
#ifndef TVN_DELETEITEM
#define TVN_DELETEITEM __MINGW_NAME_AW(TVN_DELETEITEM)
#endif
#ifndef TVN_DELETEITEMW
#define TVN_DELETEITEMW (TVN_FIRST-58)
#endif
#ifndef TVN_GETDISPINFO
#define TVN_GETDISPINFO __MINGW_NAME_AW(TVN_GETDISPINFO)
#endif
#ifndef TVN_GETDISPINFOW
#define TVN_GETDISPINFOW (TVN_FIRST-52)
#endif
#ifndef TVN_ITEMEXPANDED
#define TVN_ITEMEXPANDED __MINGW_NAME_AW(TVN_ITEMEXPANDED)
#endif
#ifndef TVN_ITEMEXPANDEDW
#define TVN_ITEMEXPANDEDW (TVN_FIRST-55)
#endif
#ifndef TVN_ITEMEXPANDING
#define TVN_ITEMEXPANDING __MINGW_NAME_AW(TVN_ITEMEXPANDING)
#endif
#ifndef TVN_ITEMEXPANDINGW
#define TVN_ITEMEXPANDINGW (TVN_FIRST-54)
#endif
#ifndef TVN_SELCHANGEDA
#define TVN_SELCHANGEDA (TVN_FIRST-2)
#endif
#ifndef TVN_SELCHANGEDW
#define TVN_SELCHANGEDW (TVN_FIRST-51)
#endif
#ifndef TVN_SELCHANGINGA
#define TVN_SELCHANGINGA (TVN_FIRST-1)
#endif
#ifndef TVN_SELCHANGINGW
#define TVN_SELCHANGINGW (TVN_FIRST-50)
#endif
#ifndef TVSIL_NORMAL
#define TVSIL_NORMAL 0
#endif
#ifndef TVSIL_STATE
#define TVSIL_STATE 2
#endif
#ifndef TranslateAccelerator
#define TranslateAccelerator __MINGW_NAME_AW(TranslateAccelerator)
#endif
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) {(P) = (P);}
#endif
#ifndef VOID
#define VOID void
#endif
#ifndef UnlockResource
#define UnlockResource(hResData) ( { (VOID)(hResData); 0; } )
#endif
#ifndef VARIABLE_PITCH
#define VARIABLE_PITCH 2
#endif
#ifndef VK_CONTROL
#define VK_CONTROL 0x11
#endif
#ifndef VK_DOWN
#define VK_DOWN 0x28
#endif
#ifndef VK_ESCAPE
#define VK_ESCAPE 0x1B
#endif
#ifndef VK_LEFT
#define VK_LEFT 0x25
#endif
#ifndef VK_RETURN
#define VK_RETURN 0x0D
#endif
#ifndef VK_RIGHT
#define VK_RIGHT 0x27
#endif
#ifndef VK_SHIFT
#define VK_SHIFT 0x10
#endif
#ifndef VK_SPACE
#define VK_SPACE 0x20
#endif
#ifndef VK_TAB
#define VK_TAB 0x09
#endif
#ifndef VK_UP
#define VK_UP 0x26
#endif
#ifndef STATUS_WAIT_0
#define STATUS_WAIT_0 ((DWORD)0x00000000)
#endif
#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0 ((STATUS_WAIT_0) + 0)
#endif
#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT 258
#endif
#ifndef WHITENESS
#define WHITENESS (DWORD)0x00FF0062
#endif
#ifndef WINDING
#define WINDING 2
#endif
#ifndef WM_COPY
#define WM_COPY 0x0301
#endif
#ifndef WM_INITDIALOG
#define WM_INITDIALOG 0x0110
#endif
#ifndef WM_KEYFIRST
#define WM_KEYFIRST 0x0100
#endif
#ifndef WM_KEYLAST
#define WM_KEYLAST 0x0109
#endif
#ifndef WM_MOUSEHOVER
#define WM_MOUSEHOVER 0x02A1
#endif
#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE 0x02A3
#endif
#ifndef WM_NCMBUTTONDOWN
#define WM_NCMBUTTONDOWN 0x00A7
#endif
#ifndef WM_NEXTDLGCTL
#define WM_NEXTDLGCTL 0x0028
#endif
#ifndef WSAOVERLAPPED
#define WSAOVERLAPPED OVERLAPPED
#endif
#ifndef WS_BORDER
#define WS_BORDER 0x00800000
#endif
#ifndef WS_CHILD
#define WS_CHILD 0x40000000
#endif
#ifndef WS_CLIPCHILDREN
#define WS_CLIPCHILDREN 0x02000000
#endif
#ifndef WS_CLIPSIBLINGS
#define WS_CLIPSIBLINGS 0x04000000
#endif
#ifndef WS_DISABLED
#define WS_DISABLED 0x08000000
#endif
#ifndef WS_EX_CLIENTEDGE
#define WS_EX_CLIENTEDGE 0x00000200
#endif
#ifndef WS_EX_STATICEDGE
#define WS_EX_STATICEDGE 0x00020000
#endif
#ifndef WS_EX_TOPMOST
#define WS_EX_TOPMOST 0x00000008
#endif
#ifndef WS_GROUP
#define WS_GROUP 0x00020000
#endif
#ifndef WS_HSCROLL
#define WS_HSCROLL 0x00100000
#endif
#ifndef WS_OVERLAPPED
#define WS_OVERLAPPED 0x00000000
#endif
#ifndef WS_POPUP
#define WS_POPUP 0x80000000
#endif
#ifndef WS_TABSTOP
#define WS_TABSTOP 0x00010000
#endif
#ifndef WS_VISIBLE
#define WS_VISIBLE 0x10000000
#endif
#ifndef WS_VSCROLL
#define WS_VSCROLL 0x00200000
#endif
#ifndef _MAX_DIR
#define _MAX_DIR 256
#endif
#ifndef _MAX_DRIVE
#define _MAX_DRIVE 3
#endif
#ifndef _MAX_EXT
#define _MAX_EXT 256
#endif
#ifndef _MAX_FNAME
#define _MAX_FNAME 256
#endif
#ifndef _SH_DENYWR
#define _SH_DENYWR 0x20
#endif
#ifndef _UI16_MAX
#define _UI16_MAX 0xffffu
#endif
#ifndef _UI32_MAX
#define _UI32_MAX 0xffffffffu
#endif
#ifndef _UI64_MAX
#define _UI64_MAX 0xffffffffffffffffull
#endif
#ifndef _UI8_MAX
#define _UI8_MAX 0xffu
#endif
#ifndef __max
#define __max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef _countof
#define _countof(_Array) sizeof(*__countof_helper(_Array))
#endif
#ifndef _doserrno
#define _doserrno (*__doserrno())
#endif
#ifndef s6_bytes
#define s6_bytes u.Byte
#endif

// 477 definitions
