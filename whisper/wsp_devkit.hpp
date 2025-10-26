#ifndef WSP_DEVKIT
#define WSP_DEVKIT

#include <type_traits>
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

#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#else
#include <signal.h>
#include <unistd.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif

#include <spdlog/spdlog.h>

#define check(expr)                                                                                                    \
    ((LIKELY(expr)) ? static_cast<void>(0) : ([] {                                                                     \
        std::ostringstream oss;                                                                                        \
        oss << "check failed at " << __FILE__ << ":" << __LINE__ << ": condition (" << #expr << ") not met";           \
        throw std::runtime_error(oss.str());                                                                           \
    })())

#define ensure(expr)                                                                                                   \
    ([&]() -> bool {                                                                                                   \
        auto _ens_val = (expr);                                                                                        \
        if (_ens_val)                                                                                                  \
            return true;                                                                                               \
        static bool _ens_once = false;                                                                                 \
        if (!_ens_once)                                                                                                \
        {                                                                                                              \
            _ens_once = true;                                                                                          \
            spdlog::critical("ensure failed at {}:{}: condition ({}) not met!", __FILE__, __LINE__, #expr);            \
            DEBUG_BREAK();                                                                                             \
        }                                                                                                              \
        return false;                                                                                                  \
    }())

#else
#define check(expr) static_cast<void>(0)
#define ensure(expr) true;
#endif

#define WPROPERTY(...) /* WPROPERTY */
#define WCLASS(...)    /* WCLASS */
#define WENUM(...)     /* WENUM */
#define REFRESH(...)   /* REFRESH */

#endif
