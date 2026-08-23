// mfc_names.h — the alias layer, included ONLY by the native probe.
//
// This branch renamed every MFC/ATL symbol with an `E` prefix (ECString,
// ECFile, EAfxBeginThread, EASSERT, ...). cases.cpp, however, has to
// compile unchanged against the REAL MFC headers too, where the only
// spellings that exist are the original ones. This header is where the two
// vocabularies meet: it maps each real MFC/ATL name onto this branch's
// E-prefixed declaration, so the cases themselves stay free of per-side
// #ifdefs and the comparison really is "the same source, two libraries".
//
// Nothing here is part of simple_mfc. It is test scaffolding, and it lives
// under tests/ precisely so that no consumer can pick the un-prefixed
// spellings back up by accident.
//
// Only what cases.cpp actually names is aliased. A symbol missing from
// this file is a symbol no case exercises — either because this branch
// does not implement it (the atlcomcli.h/atlbase.h stubs) or because it
// has no comparable behaviour (see the "WHAT IS NOT HERE" note at the top
// of cases.cpp).
#pragma once

// --- afx.h: the object model, strings, exceptions, files, archives -------
using CRuntimeClass = ECRuntimeClass;
using CObject       = ECObject;
using CDumpContext  = ECDumpContext;

using CStringA = ECStringA;
using CStringW = ECStringW;
using CString  = ECString;

using CException             = ECException;
using CSimpleException       = ECSimpleException;
using CNotSupportedException = ECNotSupportedException;
using CMemoryException       = ECMemoryException;
using CFileException         = ECFileException;
using CArchiveException      = ECArchiveException;

using CFile       = ECFile;
using CStdioFile  = ECStdioFile;
using CMemFile    = ECMemFile;
using CFileFind   = ECFileFind;
using CFileStatus = ECFileStatus;
using CArchive    = ECArchive;

// Free functions. Aliased as references rather than re-declared, so a
// signature change on the E-prefixed side is a compile error here instead
// of a silently divergent second declaration.
constexpr auto& AfxThrowFileException   = EAfxThrowFileException;
constexpr auto& AfxThrowMemoryException = EAfxThrowMemoryException;

// Macros have no C++ alias form; #define is the only mapping there is.
#define RUNTIME_CLASS       ERUNTIME_CLASS
#define DECLARE_DYNAMIC     EDECLARE_DYNAMIC
#define IMPLEMENT_DYNAMIC   EIMPLEMENT_DYNAMIC
#define DECLARE_DYNCREATE   EDECLARE_DYNCREATE
#define IMPLEMENT_DYNCREATE EIMPLEMENT_DYNCREATE
#define DYNAMIC_DOWNCAST    EDYNAMIC_DOWNCAST

// --- afxcoll.h: the non-template collections -----------------------------
using POSITION = EPOSITION;
#define BEFORE_START_POSITION EBEFORE_START_POSITION

using CObList            = ECObList;
using CPtrList           = ECPtrList;
using CStringList        = ECStringList;
using CPtrArray          = ECPtrArray;
using CStringArray       = ECStringArray;
using CByteArray         = ECByteArray;
using CWordArray         = ECWordArray;
using CDWordArray        = ECDWordArray;
using CUIntArray         = ECUIntArray;
using CMapPtrToPtr       = ECMapPtrToPtr;
using CMapStringToPtr    = ECMapStringToPtr;
using CMapStringToString = ECMapStringToString;

// --- afxtempl.h: the collection templates --------------------------------
// Alias templates, so CArray<int> and CMap<K,AK,V,AV> keep their real MFC
// spelling AND their real MFC default arguments at every use site.
template <class TYPE, class ARG_TYPE = const TYPE&>
using CArray = ECArray<TYPE, ARG_TYPE>;
template <class TYPE, class ARG_TYPE = const TYPE&>
using CList = ECList<TYPE, ARG_TYPE>;
template <class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
using CMap = ECMap<KEY, ARG_KEY, VALUE, ARG_VALUE>;
template <class BASE_CLASS, class TYPE>
using CTypedPtrList = ECTypedPtrList<BASE_CLASS, TYPE>;
template <class BASE_CLASS, class TYPE>
using CTypedPtrArray = ECTypedPtrArray<BASE_CLASS, TYPE>;

// A function TEMPLATE cannot be aliased the way an object can; the cases
// call it with an explicit argument list (HashKey<LPCTSTR>(...)), which a
// macro would mangle on the '<'. Forwarding template instead.
template <class ARG_KEY>
inline UINT HashKey(ARG_KEY key) { return EHashKey<ARG_KEY>(key); }

// --- afxmt.h: synchronisation --------------------------------------------
using CSyncObject      = ECSyncObject;
using CCriticalSection = ECCriticalSection;
using CEvent           = ECEvent;
using CMutex           = ECMutex;
using CSingleLock      = ECSingleLock;

// --- afxwin.h: threads ---------------------------------------------------
using CWinThread = ECWinThread;
using AFX_THREADPROC = EAFX_THREADPROC;
// Overloaded, so the reference form above cannot be used (it would have to
// pick one overload). A macro is the only mapping that keeps both.
#define AfxBeginThread EAfxBeginThread

// --- afxsock.h: sockets --------------------------------------------------
using CAsyncSocket = ECAsyncSocket;
constexpr auto& AfxSocketInit = EAfxSocketInit;
constexpr auto& AfxSocketTerm = EAfxSocketTerm;

// --- atltime.h -----------------------------------------------------------
using CTime     = ECTime;
using CTimeSpan = ECTimeSpan;

// --- atlalloc.h / atlsimpcoll.h / atlcoll.h ------------------------------
template <typename T, int t_nFixedBytes = 128>
using CTempBuffer = ECTempBuffer<T, t_nFixedBytes>;
template <class T, class TEqual = void>
using CSimpleArray = ECSimpleArray<T, TEqual>;
template <class KEY, class VALUE, class KTraits = void, class VTraits = void>
using CRBMap = ECRBMap<KEY, VALUE, KTraits, VTraits>;

// --- atlenc.h / atlconv.h ------------------------------------------------
constexpr auto& Base64EncodeGetRequiredLength = EBase64EncodeGetRequiredLength;
constexpr auto& Base64Encode                  = EBase64Encode;
constexpr auto& AtlUnicodeToUTF8              = EAtlUnicodeToUTF8;
#define ATL_BASE64_FLAG_NONE   EATL_BASE64_FLAG_NONE
#define ATL_BASE64_FLAG_NOPAD  EATL_BASE64_FLAG_NOPAD
#define ATL_BASE64_FLAG_NOCRLF EATL_BASE64_FLAG_NOCRLF

// --- afxinet.h -----------------------------------------------------------
constexpr auto& AfxParseURL = EAfxParseURL;
#define AFX_INET_SERVICE_HTTP  EAFX_INET_SERVICE_HTTP
#define AFX_INET_SERVICE_HTTPS EAFX_INET_SERVICE_HTTPS
#define AFX_INET_SERVICE_FTP   EAFX_INET_SERVICE_FTP

// --- calling-convention markers -----------------------------------------
// Real MFC spells the __cdecl on a thread procedure AFX_CDECL; this branch
// spells it EAFX_CDECL (and, off Windows, defines __cdecl to nothing).
#define AFX_CDECL EAFX_CDECL
