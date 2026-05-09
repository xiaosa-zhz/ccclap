#pragma once
#ifndef CCCLAP_UTIL_INTERNATIONALIZATION_H
#define CCCLAP_UTIL_INTERNATIONALIZATION_H 1

#ifndef CCCLAP_DISABLE_NATIVE_LANGUAGE
#include <libintl.h>
#endif // !CCCLAP_DISABLE_NATIVE_LANGUAGE

#include <cstdio>
#include <concepts>
#include <meta>
#include <type_traits>
#include <string>

#include <fmt/core.h>
#include <fmt/xchar.h>

#include <clap/util/cstring.hh>

namespace clap::i18n {

template<std::unsigned_integral T>
struct plural {
    T n;
    friend constexpr T format_as(const plural& p) noexcept { return p.n; }
};

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
private:
    basic_cstring_view<CharT> str;
public:
    constexpr dynamic_format_cstring(basic_cstring_view<CharT> s) noexcept : str(s) {}
    dynamic_format_cstring(const dynamic_format_cstring&) = delete;
    dynamic_format_cstring& operator=(const dynamic_format_cstring&) = delete;
    constexpr basic_cstring_view<CharT> get() const noexcept { return str; }
};

constexpr unsigned long extract_plural_arg(auto&... _) noexcept {
    auto& chosen = [:[self = std::meta::current_function()] consteval {
        std::meta::info found = {};
        for (auto param : parameters_of(self)) {
            auto arg = variable_of(param);
            auto type = remove_cvref(type_of(arg));
            if (has_template_arguments(type) && template_of(type) == ^^plural) {
                if (found != std::meta::info{}) {
                    throw std::meta::exception("multiple plural arguments found", self);
                }
                found = arg;
            }
        }
        if (found == std::meta::info{}) {
            throw std::meta::exception("no plural argument found", self);
        }
        return found;
    }():];
    return static_cast<unsigned long>(chosen.n);
}

} // namespace clap::i18n::details

constexpr auto dynamic_format(cstring_view s) noexcept {
    return details::dynamic_format_cstring<char>(s);
}

template<typename CharT, typename... Args>
class basic_format_cstring
{
public:
    template<std::convertible_to<basic_cstring_view<CharT>> T>
    consteval basic_format_cstring(const T& str)
        : str_((((void)fmt::basic_format_string<CharT, Args...>(str)), str))
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
using format_cstring = std::type_identity_t<basic_format_cstring<char, Args...>>;

template<typename... Args>
std::string format(format_cstring<Args...> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    return fmt::vformat(translated_fmt, fmt::make_format_args(args...));
}

template<typename Out, typename... Args>
auto format_to(Out out, format_cstring<Args...> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    return fmt::vformat_to(std::move(out), translated_fmt, fmt::make_format_args(args...));
}

template<typename Out, typename... Args>
auto format_to_n(Out out, size_t n, format_cstring<Args...> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    return fmt::vformat_to_n(std::move(out), n, translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
void print(format_cstring<Args...> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    fmt::vprint(translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
void print(FILE* f, format_cstring<Args...> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    fmt::vprint(f, translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
void println(format_cstring<Args...> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    fmt::vprintln(stdout, translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
void println(FILE* f, format_cstring<Args...> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    fmt::vprintln(f, translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
std::string plural_format(format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
        details::extract_plural_arg(args...));
    return fmt::vformat(translated_fmt, fmt::make_format_args(args...));
}

template<typename Out, typename... Args>
auto plural_format_to(Out out, format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
        details::extract_plural_arg(args...));
    return fmt::vformat_to(std::move(out), translated_fmt, fmt::make_format_args(args...));
}

template<typename Out, typename... Args>
auto plural_format_to_n(Out out, size_t n, format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
        details::extract_plural_arg(args...));
    return fmt::vformat_to_n(std::move(out), n, translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
void plural_print(format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
        details::extract_plural_arg(args...));
    fmt::vprint(translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
void plural_print(FILE* f, format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
        details::extract_plural_arg(args...));
    fmt::vprint(f, translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
void plural_println(format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
        details::extract_plural_arg(args...));
    fmt::vprintln(stdout, translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
void plural_println(FILE* f, format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    const char* translated_fmt = details::ngettext(fmt.c_str(), fmt_plural.c_str(),
        details::extract_plural_arg(args...));
    fmt::vprintln(f, translated_fmt, fmt::make_format_args(args...));
}

} // namespace clap::i18n

#endif // !CCCLAP_UTIL_INTERNATIONALIZATION_H
