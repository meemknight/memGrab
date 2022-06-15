#pragma once

#define MAX_PATH_COMMON 256

#if defined WIN32 || defined _WIN32 || defined __WIN32__ || defined __NT__
#define NOMINMAX
#include <Windows.h>

using PID = DWORD;
using PROCESS = HANDLE;
#elif defined __linux__ 
using PID = pid_t;
using PROCESS = PID; //?
#endif

#ifdef _MSC_VER
#define IM_PRId64   "I64d"
#define IM_PRIu64   "I64u"
#define IM_PRIx64   "I64X"
#else
#define IM_PRId64   "lld"
#define IM_PRIu64   "llu"
#define IM_PRIx64   "llX"
#endif

