// afxinet.cpp — AfxParseURL. Pure string parsing (scheme/host/port/
// object), no WinInet dependency.
#include "eafxinet.h"

BOOL EAfxParseURL(LPCTSTR pstrURL, DWORD& dwServiceType, ECString& strServer,
                 ECString& strObject, INTERNET_PORT& nPort)
{
    if (pstrURL == nullptr)
        return FALSE;
    ECString url(pstrURL);
    if (url.IsEmpty())
        return FALSE;

    // A scheme ("http://") is REQUIRED: a URL without one makes real MFC
    // return FALSE, and that failure is load-bearing. eMule's only caller
    // (HttpDownloadDlg.cpp) relies on it precisely -- it calls AfxParseURL,
    // and *on failure* prepends "http://" and retries. An earlier version
    // here defaulted a schemeless URL to HTTP and returned TRUE, which
    // silently suppressed that retry and left eMule's download URL without
    // the prefix its own logic expects. Verified against real MFC by the
    // conformance suite (AfxParseURL.schemeless.fails).
    int nSchemeEnd = url.Find(L"://");
    if (nSchemeEnd < 0)
        return FALSE;
    ECString scheme = url.Left(nSchemeEnd);
    scheme.MakeLower();
    ECString rest = url.Mid(nSchemeEnd + 3);

    INTERNET_PORT nDefaultPort;
    if (scheme == ECString(L"https")) { dwServiceType = EAFX_INET_SERVICE_HTTPS; nDefaultPort = 443; }
    else if (scheme == ECString(L"ftp")) { dwServiceType = EAFX_INET_SERVICE_FTP; nDefaultPort = 21; }
    else { dwServiceType = EAFX_INET_SERVICE_HTTP; nDefaultPort = 80; }

    // host[:port] up to the first '/', which starts the object (path).
    int nSlash = rest.Find(L'/');
    ECString hostPort = (nSlash >= 0) ? rest.Left(nSlash) : rest;
    strObject = (nSlash >= 0) ? rest.Mid(nSlash) : ECString(L"/");
    if (strObject.IsEmpty())
        strObject = L"/";

    int nColon = hostPort.ReverseFind(L':');
    if (nColon >= 0)
    {
        strServer = hostPort.Left(nColon);
        ECString portStr = hostPort.Mid(nColon + 1);
        long nParsedPort = 0;
        bool bValid = !portStr.IsEmpty();
        for (int i = 0; i < portStr.GetLength() && bValid; ++i)
        {
            wchar_t c = portStr[i];
            if (c < L'0' || c > L'9') { bValid = false; break; }
            nParsedPort = nParsedPort * 10 + (c - L'0');
        }
        nPort = (bValid && nParsedPort > 0) ? static_cast<INTERNET_PORT>(nParsedPort) : nDefaultPort;
    }
    else
    {
        strServer = hostPort;
        nPort = nDefaultPort;
    }

    return strServer.IsEmpty() ? FALSE : TRUE;
}
