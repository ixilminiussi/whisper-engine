#ifndef WSP_DEVKIT
#define WSP_DEVKIT

#ifndef NDEBUG
#include <sstream>
#include <stdexcept>

#if defined(__GNUC__) || defined(__clang__)
#define LIKELY(x) (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

#define check(expr)                                                                                                    \
    ((LIKELY(expr)) ? static_cast<void>(0) : ([] {                                                                     \
        std::ostringstream oss;                                                                                        \
        oss << "check failed at " << __FILE__ << ":" << __LINE__ << ": condition (" << #expr << ") not met";           \
        throw std::runtime_error(oss.str());                                                                           \
    })())
#else
#define check(expr) static_cast<void>(0)
#endif

#define PROPERTY(...) [[clang::annotate("GROUP_PROPERTY")]]

#endif
