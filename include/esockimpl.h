#pragma once
#include "eafxsock.h"

struct E_AFX_SOCK_STATE
{
    void(EAFX_CDECL* m_pfnSockTerm)();
};

struct E_AFX_SOCK_STATE_HOLDER
{
    E_AFX_SOCK_STATE* GetData();
};
extern E_AFX_SOCK_STATE_HOLDER E_afxSockState;
