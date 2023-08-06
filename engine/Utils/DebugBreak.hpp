#ifdef _MSC_VER
#define DEBUG_BREAK() __debugbreak()
#else
// No cross platform way to do it.
#define DEBUG_BREAK()
#endif
