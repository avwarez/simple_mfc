#pragma once

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

constexpr auto& AfxThrowFileException   = EAfxThrowFileException;
constexpr auto& AfxThrowMemoryException = EAfxThrowMemoryException;

#define RUNTIME_CLASS       ERUNTIME_CLASS
#define DECLARE_DYNAMIC     EDECLARE_DYNAMIC
#define IMPLEMENT_DYNAMIC   EIMPLEMENT_DYNAMIC
#define DECLARE_DYNCREATE   EDECLARE_DYNCREATE
#define IMPLEMENT_DYNCREATE EIMPLEMENT_DYNCREATE
#define DYNAMIC_DOWNCAST    EDYNAMIC_DOWNCAST

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

template <class ARG_KEY>
inline UINT HashKey(ARG_KEY key) { return EHashKey<ARG_KEY>(key); }

using CSyncObject      = ECSyncObject;
using CCriticalSection = ECCriticalSection;
using CEvent           = ECEvent;
using CMutex           = ECMutex;
using CSingleLock      = ECSingleLock;

using CWinThread = ECWinThread;
using AFX_THREADPROC = EAFX_THREADPROC;
#define AfxBeginThread EAfxBeginThread

using CAsyncSocket = ECAsyncSocket;
constexpr auto& AfxSocketInit = EAfxSocketInit;
constexpr auto& AfxSocketTerm = EAfxSocketTerm;

using CTime     = ECTime;
using CTimeSpan = ECTimeSpan;

template <typename T, int t_nFixedBytes = 128>
using CTempBuffer = ECTempBuffer<T, t_nFixedBytes>;
template <class T, class TEqual = void>
using CSimpleArray = ECSimpleArray<T, TEqual>;
template <class KEY, class VALUE, class KTraits = void, class VTraits = void>
using CRBMap = ECRBMap<KEY, VALUE, KTraits, VTraits>;

constexpr auto& Base64EncodeGetRequiredLength = EBase64EncodeGetRequiredLength;
constexpr auto& Base64Encode                  = EBase64Encode;
constexpr auto& AtlUnicodeToUTF8              = EAtlUnicodeToUTF8;
constexpr auto& _AtlGetConversionACP          = E_AtlGetConversionACP;
#define ATL_BASE64_FLAG_NONE   EATL_BASE64_FLAG_NONE
#define ATL_BASE64_FLAG_NOPAD  EATL_BASE64_FLAG_NOPAD
#define ATL_BASE64_FLAG_NOCRLF EATL_BASE64_FLAG_NOCRLF

constexpr auto& AfxParseURL = EAfxParseURL;
#define AFX_INET_SERVICE_HTTP  EAFX_INET_SERVICE_HTTP
#define AFX_INET_SERVICE_HTTPS EAFX_INET_SERVICE_HTTPS
#define AFX_INET_SERVICE_FTP   EAFX_INET_SERVICE_FTP

#define AFX_CDECL EAFX_CDECL
