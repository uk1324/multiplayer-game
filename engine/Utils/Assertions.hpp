#pragma once

// ASSERT - Crash.
// CHECK - Just for debug checks. Should not happen, but if it does it is handled.

#ifdef CLIENT
void assertImplementation(bool condition, const char* functionName, int line);

#if defined(__GNUC__) || defined(__GNUG__) 
#define FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define FUNCTION_SIGNATURE __FUNCSIG__
#endif

#define ASSERT(condition) assertImplementation(condition, FUNCTION_SIGNATURE, __LINE__)
#define ASSERT_NOT_REACHED() ASSERT(false)

#ifdef FINAL_RELEASE
#define CHECK(condition)
#else
#define CHECK(condition) assertImplementation(condition, FUNCTION_SIGNATURE, __LINE__)
#endif

#define CHECK_NOT_REACHED() CHECK(false)

#else

#include <assert.h>

#define ASSERT(condition) assert(condition)
#define ASSERT_NOT_REACHED() assert(0)

#define CHECK(condition) assert(condition)
#define CHECK_NOT_REACHED() assert(0)

#endif