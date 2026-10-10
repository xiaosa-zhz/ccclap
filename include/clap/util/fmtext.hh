#pragma once
#ifndef CCCLAP_UTIL_FMTEXT_H
#define CCCLAP_UTIL_FMTEXT_H 1

#ifndef CCCLAP_DISABLE_NATIVE_LANGUAGE
#include <libintl.h>
#endif // !CCCLAP_DISABLE_NATIVE_LANGUAGE

#include <cstdio>
#include <concepts>
#include <meta>
#include <type_traits>
#include <string>
#include <iterator>
#include <format>
#include <print>

#include <clap/util/cstring.hh>
#include <clap/util/metax.hh>

namespace clap::fmtext {

template<std::unsigned_integral T>
struct plural { T n; };

namespace details {

#ifdef CCCLAP_DISABLE_NATIVE_LANGUAGE

constexpr const char* gettext(const char* msgid) noexcept {
    return msgid;
}

constexpr const char* ngettext(const char* msgid, const char* msgid_plural, unsigned long n) noexcept {
    return n == 1 ? msgid : msgid_plural;
}

#else // vvv !CCCLAP_DISABLE_NATIVE_LANGUAGE

inline const char* gettext(const char* msgid) noexcept {
    return ::gettext(msgid);
}

inline const char* ngettext(const char* msgid, const char* msgid_plural, unsigned long n) noexcept {
    return ::ngettext(msgid, msgid_plural, n);
}

#endif // CCCLAP_DISABLE_NATIVE_LANGUAGE

template<typename CharT>
class dynamic_format_cstring
{
public:
    constexpr dynamic_format_cstring(basic_cstring_view<CharT> s) noexcept : str_(s) {}
    dynamic_format_cstring(const dynamic_format_cstring&) = delete;
    dynamic_format_cstring& operator=(const dynamic_format_cstring&) = delete;
    constexpr basic_cstring_view<CharT> get() const noexcept { return str_; }
private:
    basic_cstring_view<CharT> str_;
};

constexpr unsigned long extract_plural_arg(auto&...) noexcept {
    auto& chosen = [:[self = std::meta::current_function()] consteval {
        std::meta::info found = {};
        for (auto param : parameters_of(self)) {
            const auto arg = variable_of(param);
            const auto type = remove_cvref(type_of(arg));
            if (metax::is_specialization_of(type, ^^plural)) {
                if (found != metax::null) {
                    throw std::meta::exception("multiple plural arguments found", self);
                }
                found = arg;
            }
        }
        if (found == metax::null) {
            throw std::meta::exception("no plural argument found", self);
        }
        return found;
    }():];
    return static_cast<unsigned long>(chosen.n);
}

} // namespace clap::fmtext::details

constexpr auto dynamic_format(cstring_view s) noexcept {
    return details::dynamic_format_cstring<char>(s);
}

// Why cstring_view? Because it guarantees null-termination, which is required for gettext.
template<typename CharT, typename... Args>
class basic_format_cstring
{
public:
    basic_format_cstring() = delete;
    constexpr basic_format_cstring(const basic_format_cstring&) = default;
    constexpr basic_format_cstring& operator=(const basic_format_cstring&) = default;

    template<std::convertible_to<basic_cstring_view<CharT>> T>
    consteval basic_format_cstring(const T& str)
        : str_((((void)std::basic_format_string<CharT, Args...>(str)), str))
    {}

    constexpr basic_format_cstring(details::dynamic_format_cstring<CharT> s) noexcept
        : str_(s.get())
    {}

    constexpr auto get() const noexcept -> basic_cstring_view<CharT> { return str_; }
    constexpr const CharT* c_str() const noexcept { return str_.c_str(); }

private:
    basic_cstring_view<CharT> str_;
};

template<typename... Args>
using format_cstring = [:^^basic_format_cstring<char, Args...>:];

template<typename... Args>
constexpr std::string format(format_cstring<Args...> fmt, Args&&... args) {
    if consteval {
        return std::format(std::dynamic_format(fmt.get()), std::forward<Args>(args)...);
    } else {
        const char* translated_fmt = details::gettext(fmt.c_str());
        return std::vformat(translated_fmt, std::make_format_args(args...));
    }
}

template<std::output_iterator<char> Out, typename... Args>
constexpr Out format_to(Out out, format_cstring<Args...> fmt, Args&&... args) {
    if consteval {
        return std::format_to(std::move(out), std::dynamic_format(fmt.get()), std::forward<Args>(args)...);
    } else {
        const char* translated_fmt = details::gettext(fmt.c_str());
        return std::vformat_to(std::move(out), translated_fmt, std::make_format_args(args...));
    }
}

template<typename Container, typename... Args>
    requires requires (Container& c, char ch) { c.push_back(ch); }
constexpr auto format_append(Container& c, format_cstring<Args...> fmt, Args&&... args) {
    return format_to(std::back_inserter(c), fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void print(format_cstring<Args...> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    std::print(std::dynamic_format(translated_fmt), args...);
}

template<typename... Args>
void print(FILE* f, format_cstring<Args...> fmt, Args&&... args)
    pre (f != nullptr)
{
    const char* translated_fmt = details::gettext(fmt.c_str());
    std::print(f, std::dynamic_format(translated_fmt), args...);
}

template<typename... Args>
void println(format_cstring<Args...> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    std::println(std::dynamic_format(translated_fmt), args...);
}

template<typename... Args>
void println(FILE* f, format_cstring<Args...> fmt, Args&&... args)
    pre (f != nullptr)
{
    const char* translated_fmt = details::gettext(fmt.c_str());
    std::println(f, std::dynamic_format(translated_fmt), args...);
}

template<typename... Args>
constexpr std::string plural_format(format_cstring<Args...> fmt,
                                    format_cstring<Args...> fmt_plural,
                                    Args&&... args) {
    if consteval {
        unsigned long n = details::extract_plural_arg(args...);
        auto chosen = (n == 1) ? fmt.get() : fmt_plural.get();
        return std::format(std::dynamic_format(chosen), std::forward<Args>(args)...);
    } else {
        const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
            details::extract_plural_arg(args...));
        return std::vformat(translated_fmt, std::make_format_args(args...));
    }
}

template<std::output_iterator<char> Out, typename... Args>
constexpr auto plural_format_to(Out out,
                                format_cstring<Args...> fmt,
                                format_cstring<Args...> fmt_plural,
                                Args&&... args) {
    if consteval {
        unsigned long n = details::extract_plural_arg(args...);
        auto chosen = (n == 1) ? fmt.get() : fmt_plural.get();
        return std::format_to(std::move(out), std::dynamic_format(chosen), std::forward<Args>(args)...);
    } else {
        const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
            details::extract_plural_arg(args...));
        return std::vformat_to(std::move(out), translated_fmt, std::make_format_args(args...));
    }
}

template<typename... Args>
void plural_print(format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
        details::extract_plural_arg(args...));
    std::print(std::dynamic_format(translated_fmt), args...);
}

template<typename... Args>
void plural_print(FILE* f,
                  format_cstring<Args...> fmt,
                  format_cstring<Args...> fmt_plural,
                  Args&&... args)
    pre (f != nullptr)
{
    const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
        details::extract_plural_arg(args...));
    std::print(f, std::dynamic_format(translated_fmt), args...);
}

template<typename... Args>
void plural_println(format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
        details::extract_plural_arg(args...));
    std::println(std::dynamic_format(translated_fmt), args...);
}

template<typename... Args>
void plural_println(FILE* f,
                    format_cstring<Args...> fmt,
                    format_cstring<Args...> fmt_plural,
                    Args&&... args)
    pre (f != nullptr)
{
    const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
        details::extract_plural_arg(args...));
    std::println(f, std::dynamic_format(translated_fmt), args...);
}

constexpr std::string to_string(int value) { return format("{}", value); }
constexpr std::string to_string(long value) { return format("{}", value); }
constexpr std::string to_string(long long value) { return format("{}", value); }
constexpr std::string to_string(unsigned int value) { return format("{}", value); }
constexpr std::string to_string(unsigned long value) { return format("{}", value); }
constexpr std::string to_string(unsigned long long value) { return format("{}", value); }

} // namespace clap::fmtext

template<typename T>
struct std::formatter<clap::fmtext::plural<T>, char> : std::formatter<T, char> {
    template <typename FormatContext>
    constexpr auto format(clap::fmtext::plural<T> p, FormatContext& ctx) const {
        using base = std::formatter<T, char>;
        return base::format(p.n, ctx);
    }
};

#endif // !CCCLAP_UTIL_FMTEXT_H
