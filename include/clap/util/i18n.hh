#pragma once
#ifndef CCCLAP_UTIL_INTERNATIONALIZATION_H
#define CCCLAP_UTIL_INTERNATIONALIZATION_H 1

#ifndef CCCLAP_DISABLE_NATIVE_LANGUAGE
#include <libintl.h>
#endif // !CCCLAP_DISABLE_NATIVE_LANGUAGE

#include <cstdio>
#include <concepts>
#include <type_traits>
#include <string>

#include <fmt/core.h>
#include <fmt/xchar.h>

#include <clap/util/cstring.hh>

namespace clap::i18n {

namespace details {

#ifdef CCCLAP_DISABLE_NATIVE_LANGUAGE

constexpr const char* gettext(const char* msgid) noexcept {
    return msgid;
}

#else // vvv !CCCLAP_DISABLE_NATIVE_LANGUAGE

inline const char* gettext(const char* msgid) noexcept {
    return ::gettext(msgid);
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
using format_cstring = basic_format_cstring<char, Args...>;

template<typename... Args>
std::string format(std::type_identity_t<format_cstring<Args...>> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    return fmt::vformat(translated_fmt, fmt::make_format_args(args...));
}

template<typename Out, typename... Args>
auto format_to(Out out, std::type_identity_t<format_cstring<Args...>> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    return fmt::vformat_to(std::move(out), translated_fmt, fmt::make_format_args(args...));
}

template<typename Out, typename... Args>
auto format_to_n(Out out, size_t n, std::type_identity_t<format_cstring<Args...>> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    return fmt::vformat_to_n(std::move(out), n, translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
void print(std::type_identity_t<format_cstring<Args...>> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    fmt::vprint(translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
void print(FILE* f, std::type_identity_t<format_cstring<Args...>> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    fmt::vprint(f, translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
void println(std::type_identity_t<format_cstring<Args...>> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    fmt::vprintln(stdout, translated_fmt, fmt::make_format_args(args...));
}

template<typename... Args>
void println(FILE* f, std::type_identity_t<format_cstring<Args...>> fmt, Args&&... args) {
    const char* translated_fmt = details::gettext(fmt.c_str());
    fmt::vprintln(f, translated_fmt, fmt::make_format_args(args...));
}

} // namespace clap::i18n

#endif // !CCCLAP_UTIL_INTERNATIONALIZATION_H
