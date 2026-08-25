#pragma once
#include "eafx.h"

struct ECTraceCategory
{
    explicit ECTraceCategory(UINT nCategory = 0) noexcept;
};
extern ECTraceCategory EtraceAppMsg;
