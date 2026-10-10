#pragma once
#ifndef CCCLAP_UTIL_FMTEXT_H
#define CCCLAP_UTIL_FMTEXT_H 1

#ifndef CCCLAP_DISABLE_NATIVE_LANGUAGE
#include <libintl.h>
#endif // !CCCLAP_DISABLE_NATIVE_LANGUAGE

#include <cstdio>
#include <concepts>
#include <meta>
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

constexpr cstring_view gettext(cstring_view msgid) noexcept {
#ifdef CCCLAP_DISABLE_NATIVE_LANGUAGE
    return msgid;
#else // vvv !CCCLAP_DISABLE_NATIVE_LANGUAGE
    if consteval {
        return msgid;
    } else {
        return ::gettext(msgid.c_str());
    }
#endif // CCCLAP_DISABLE_NATIVE_LANGUAGE
}

constexpr cstring_view ngettext(cstring_view msgid, cstring_view msgid_plural, unsigned long n) noexcept {
#ifdef CCCLAP_DISABLE_NATIVE_LANGUAGE
    return n == 1 ? msgid : msgid_plural;
#else // vvv !CCCLAP_DISABLE_NATIVE_LANGUAGE
    if consteval {
        return n == 1 ? msgid : msgid_plural;
    } else {
        return ::ngettext(msgid.c_str(), msgid_plural.c_str(), n);
    }
#endif // CCCLAP_DISABLE_NATIVE_LANGUAGE
}

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

constexpr unsigned long extract_plural_arg(const auto&...) noexcept {
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

constexpr auto translate(cstring_view s) noexcept {
    return std::dynamic_format(gettext(s));
}

constexpr auto translate(cstring_view msgid, cstring_view msgid_plural, const auto&... args) noexcept {
    const auto n = extract_plural_arg(args...);
    return std::dynamic_format(ngettext(msgid, msgid_plural, n));
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
    constexpr operator basic_cstring_view<CharT>() const noexcept { return get(); }

private:
    basic_cstring_view<CharT> str_;
};

template<typename... Args>
using format_cstring = [:^^basic_format_cstring<char, Args...>:];

template<typename... Args>
constexpr std::string format(format_cstring<Args...> fmt, Args&&... args) {
    return std::format(details::translate(fmt), std::forward<Args>(args)...);
}

template<std::output_iterator<char> Out, typename... Args>
constexpr Out format_to(Out out, format_cstring<Args...> fmt, Args&&... args) {
    return std::format_to(std::move(out), details::translate(fmt), std::forward<Args>(args)...);
}

template<typename Container, typename... Args>
    requires requires (Container& c, char ch) { c.push_back(ch); }
constexpr auto format_append(Container& c, format_cstring<Args...> fmt, Args&&... args) {
    return format_to(std::back_inserter(c), fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void print(format_cstring<Args...> fmt, Args&&... args) {
    std::print(details::translate(fmt), std::forward<Args>(args)...);
}

template<typename... Args>
void print(FILE* f, format_cstring<Args...> fmt, Args&&... args)
    pre (f != nullptr)
{
    std::print(f, details::translate(fmt), std::forward<Args>(args)...);
}

template<typename... Args>
void println(format_cstring<Args...> fmt, Args&&... args) {
    std::println(details::translate(fmt), std::forward<Args>(args)...);
}

template<typename... Args>
void println(FILE* f, format_cstring<Args...> fmt, Args&&... args)
    pre (f != nullptr)
{
    std::println(f, details::translate(fmt), std::forward<Args>(args)...);
}

template<typename... Args>
constexpr std::string plural_format(format_cstring<Args...> fmt,
                                    format_cstring<Args...> fmt_plural,
                                    Args&&... args) {
    return std::format(details::translate(fmt, fmt_plural, args...), std::forward<Args>(args)...);
}

template<std::output_iterator<char> Out, typename... Args>
constexpr auto plural_format_to(Out out,
                                format_cstring<Args...> fmt,
                                format_cstring<Args...> fmt_plural,
                                Args&&... args) {
    return std::format_to(std::move(out), details::translate(fmt, fmt_plural, args...), std::forward<Args>(args)...);
}

template<typename... Args>
void plural_print(format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    std::print(details::translate(fmt, fmt_plural, args...), std::forward<Args>(args)...);
}

template<typename... Args>
void plural_print(FILE* f,
                  format_cstring<Args...> fmt,
                  format_cstring<Args...> fmt_plural,
                  Args&&... args)
    pre (f != nullptr)
{
    std::print(f, details::translate(fmt, fmt_plural, args...), std::forward<Args>(args)...);
}

template<typename... Args>
void plural_println(format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    std::println(details::translate(fmt, fmt_plural, args...), std::forward<Args>(args)...);
}

template<typename... Args>
void plural_println(FILE* f,
                    format_cstring<Args...> fmt,
                    format_cstring<Args...> fmt_plural,
                    Args&&... args)
    pre (f != nullptr)
{
    std::println(f, details::translate(fmt, fmt_plural, args...), std::forward<Args>(args)...);
}

} // namespace clap::fmtext

template<typename T>
struct std::formatter<clap::fmtext::plural<T>, char> : std::formatter<T, char> {
    template <typename FormatContext>
    constexpr auto format(clap::fmtext::plural<T> p, FormatContext& ctx) const {
        return std::formatter<T, char>::format(p.n, ctx);
    }
};

#endif // !CCCLAP_UTIL_FMTEXT_H
