#include "stdafx.h"
#include "ThreadLocal.h"

__declspec(thread) int LThreadType = -1;
__declspec(thread) int LIoThreadId = -1;

__declspec( thread ) int LSendCount;
__declspec( thread ) int LRecvCount;