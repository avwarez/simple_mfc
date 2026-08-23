#pragma once

#ifndef EAFX_INET_SERVICE_HTTP
#define EAFX_INET_SERVICE_HTTP 3
#endif
#ifndef EAFX_INET_SERVICE_HTTPS
#define EAFX_INET_SERVICE_HTTPS 4
#endif
#ifndef EAFX_INET_SERVICE_FTP
#define EAFX_INET_SERVICE_FTP 1
#endif

#include "eafx.h"

#ifdef _WIN32
#include <wininet.h>
#else
using INTERNET_PORT = unsigned short;
#endif

BOOL EAfxParseURL(LPCTSTR pstrURL, DWORD& dwServiceType, ECString& strServer,
                 ECString& strObject, INTERNET_PORT& nPort);
