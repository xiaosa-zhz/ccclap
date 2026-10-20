#pragma once
#ifndef CCCLAP_UTIL_MISC_H
#define CCCLAP_UTIL_MISC_H 1

#include <type_traits>
#include <utility>
#include <functional>

namespace clap::util {

struct moveonly {
    moveonly() = default;
    moveonly(const moveonly&) = delete;
    moveonly& operator=(const moveonly&) = delete;
    moveonly(moveonly&&) = default;
    moveonly& operator=(moveonly&&) = default;
};

struct pinned : moveonly {
    pinned() = default;
    pinned(pinned&&) = delete;
    pinned& operator=(pinned&&) = delete;
};

struct rooted : pinned {
    rooted() = delete;
};

template<typename T, typename F>
    requires std::is_invocable_r_v<T, F>
struct lazy
{
    std::remove_cvref_t<F> func;
    T get() { return std::invoke_r<T>(std::forward<F>(func)); }
    operator T() { return get(); }
};

template<typename T, typename F>
lazy<T, F> make_lazy(F&& func) {
    return lazy<T, F>{std::forward<F>(func)};
}

} // namespace clap::util

#endif // !CCCLAP_UTIL_MISC_H
