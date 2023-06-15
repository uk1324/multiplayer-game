#include <assert.h>

// Crash if happens
#define ASSERT(condition) assert(condition)
#define ASSERT_NOT_REACHED() assert(0)

// Just for debug checks. Should happen, but if it does it is handled.
#define CHECK(condition) assert(condition)
#define CHECK_NOT_REACHED() assert(0)