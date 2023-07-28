#pragma once

// ASSERT - Crash.
// CHECK - Just for debug checks. Should not happen, but if it does it is handled.

#ifdef CLIENT
void assertImplementation(bool condition, const char* functionName, int line);

#define ASSERT(condition) assertImplementation(condition, __FUNCSIG__, __LINE__)
#define ASSERT_NOT_REACHED() ASSERT(false)

#ifdef FINAL_RELEASE
#define CHECK(condition)
#else
#define CHECK(condition) assertImplementation(condition, __FUNCSIG__, __LINE__)
#endif

#define CHECK_NOT_REACHED() CHECK(false)

#else

#include <assert.h>

#define ASSERT(condition) assert(condition)
#define ASSERT_NOT_REACHED() assert(0)

#define CHECK(condition) assert(condition)
#define CHECK_NOT_REACHED() assert(0)

#endif