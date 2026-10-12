#pragma once
#ifndef CCCLAP_UTIL_I18N_H
#define CCCLAP_UTIL_I18N_H 1

#if !defined(CCCLAP_DISABLE_I18N) && __has_include(<libintl.h>)
#include <libintl.h>
#include <locale.h>
#define CCCLAP_HAS_LIBINTL 1
#endif // !defined(CCCLAP_DISABLE_I18N) && __has_include(<libintl.h>)

#include <cstdio>
#include <concepts>
#include <meta>
#include <utility>
#include <locale>
#include <optional>
#include <string>
#include <iterator>
#include <format>
#include <print>

#include <clap/util/cstring.hh>
#include <clap/util/metax.hh>

namespace clap::i18n {

template<std::unsigned_integral T>
struct plural { T n; };

struct i18n_args {
    cstring_view domain = {};
    int category = -1;
    std::optional<const std::locale&> locale = std::nullopt;
};

namespace details {

class posix_locale_guard
{
public:
    posix_locale_guard(const posix_locale_guard&) = delete;
    posix_locale_guard& operator=(const posix_locale_guard&) = delete;

    static constexpr posix_locale_guard make(const std::locale& std_locale) {
        if consteval {
            throw std::meta::exception("Should not use locale during constant evaluation", {});
        } else {
            return posix_locale_guard(std_locale.name());
        }
    }

    posix_locale_guard(cstring_view name) noexcept {
        ::locale_t locale = ::newlocale(LC_ALL_MASK, name.c_str(), nullptr);
        if (locale != nullptr) {
            old_locale_ = ::uselocale(locale);
        }
    }

    constexpr ~posix_locale_guard() {
        if !consteval {
            if (old_locale_ != nullptr) {
                ::freelocale(::uselocale(old_locale_));
            }
        }
    }

private:
    ::locale_t old_locale_ = nullptr;
};

constexpr cstring_view gettext(const i18n_args& ia, cstring_view msgid) noexcept {
#if !defined(CCCLAP_HAS_LIBINTL)
    return msgid;
#else // defined(CCCLAP_HAS_LIBINTL)
    if consteval {
        return msgid;
    } else {
        const auto guard = ia.locale.transform(&posix_locale_guard::make);
        if (!ia.domain.empty()) {
            if (ia.category != -1) {
                return ::dcgettext(ia.domain.c_str(),
                                msgid.c_str(),
                                ia.category);
            } else {
                return ::dgettext(ia.domain.c_str(), msgid.c_str());
            }
        } else {
            return ::gettext(msgid.c_str());
        }
    }
#endif // !defined(CCCLAP_HAS_LIBINTL)
}

constexpr cstring_view ngettext(const i18n_args& ia,
                                cstring_view msgid,
                                cstring_view msgid_plural,
                                unsigned long n) noexcept {
#if !defined(CCCLAP_HAS_LIBINTL)
    return n == 1 ? msgid : msgid_plural;
#else // defined(CCCLAP_HAS_LIBINTL)
    if consteval {
        return n == 1 ? msgid : msgid_plural;
    } else {
        const auto guard = ia.locale.transform(&posix_locale_guard::make);
        if (!ia.domain.empty()) {
            if (ia.category != -1) {
                return ::dcngettext(ia.domain.c_str(),
                                    msgid.c_str(), msgid_plural.c_str(), n,
                                    ia.category);
            } else {
                return ::dngettext(ia.domain.c_str(),
                                   msgid.c_str(), msgid_plural.c_str(), n);
            }
        } else {
            return ::ngettext(msgid.c_str(), msgid_plural.c_str(), n);
        }
    }
#endif // !defined(CCCLAP_HAS_LIBINTL)
}

template<typename CharT>
class dynamic_format_cstring
{
public:
    using cstring_view = basic_cstring_view<CharT>;

    constexpr dynamic_format_cstring(cstring_view s) noexcept : fmt_(s) {}
    dynamic_format_cstring(const dynamic_format_cstring&) = delete;
    dynamic_format_cstring& operator=(const dynamic_format_cstring&) = delete;
    constexpr cstring_view get() const noexcept { return fmt_; }
private:
    cstring_view fmt_;
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

template<typename CharT>
constexpr auto translate(const i18n_args& ia, basic_cstring_view<CharT> msgid) noexcept {
    return std::dynamic_format(gettext(ia, msgid));
}

template<typename CharT>
constexpr auto translate(const i18n_args& ia,
                         basic_cstring_view<CharT> msgid,
                         basic_cstring_view<CharT> msgid_plural,
                         const auto&... args) noexcept {
    const auto n = extract_plural_arg(args...);
    return std::dynamic_format(ngettext(ia, msgid, msgid_plural, n));
}

inline std::string& local_buffer() {
    thread_local std::string buf;
    buf.clear();
    return buf;
}

} // namespace clap::i18n::details

constexpr auto dynamic_format(cstring_view s) noexcept {
    return details::dynamic_format_cstring<char>(s);
}

// Why cstring_view? Because it guarantees null-termination, which is required for gettext.
template<typename CharT, typename... Args>
class basic_format_cstring
{
public:
    using cstring_view = basic_cstring_view<CharT>;

    basic_format_cstring() = delete;
    constexpr basic_format_cstring(const basic_format_cstring&) = default;
    constexpr basic_format_cstring& operator=(const basic_format_cstring&) = default;

    template<std::convertible_to<cstring_view> FMT>
    consteval basic_format_cstring(const FMT& fmt)
        : fmt_((((void)std::basic_format_string<CharT, Args...>(fmt)), fmt))
    {}

    constexpr basic_format_cstring(details::dynamic_format_cstring<CharT> s) noexcept
        : fmt_(s.get())
    {}

    constexpr cstring_view get() const noexcept { return fmt_; }
    constexpr const CharT* c_str() const noexcept { return fmt_.c_str(); }

private:
    cstring_view fmt_;
};

template<typename... Args>
using format_cstring = [:^^basic_format_cstring<char, Args...>:];

template<typename... Args>
constexpr std::string format(format_cstring<Args...> fmt, Args&&... args) {
    return std::format(details::translate({}, fmt.get()), std::forward<Args>(args)...);
}

template<typename... Args>
constexpr std::string format(const i18n_args& ia, format_cstring<Args...> fmt, Args&&... args) {
    if (ia.locale) {
        return std::format(*ia.locale, details::translate(ia, fmt.get()), std::forward<Args>(args)...);
    } else {
        return std::format(details::translate(ia, fmt.get()), std::forward<Args>(args)...);
    }
}

template<std::output_iterator<char> Out, typename... Args>
constexpr Out format_to(Out out, format_cstring<Args...> fmt, Args&&... args) {
    return std::format_to(std::move(out), details::translate({}, fmt.get()), std::forward<Args>(args)...);
}

template<std::output_iterator<char> Out, typename... Args>
constexpr Out format_to(Out out, const i18n_args& ia, format_cstring<Args...> fmt, Args&&... args) {
    if (ia.locale) {
        return std::format_to(std::move(out), *ia.locale, details::translate(ia, fmt.get()), std::forward<Args>(args)...);
    } else {
        return std::format_to(std::move(out), details::translate(ia, fmt.get()), std::forward<Args>(args)...);
    }
}

namespace details {

template<typename... Args>
cstring_view format_to_local_buffer(const i18n_args& ia, format_cstring<Args...> fmt, Args&&... args)
    pre (ia.locale.has_value())
{
    std::string& buf = local_buffer();
    i18n::format_to(std::back_inserter(buf), ia, fmt, std::forward<Args>(args)...);
    return buf;
}

} // namespace clap::i18n::details

template<typename... Args>
void print(format_cstring<Args...> fmt, Args&&... args) {
    std::print(details::translate({}, fmt.get()), std::forward<Args>(args)...);
}

template<typename... Args>
void print(const i18n_args& ia, format_cstring<Args...> fmt, Args&&... args) {
    if (ia.locale) {
        std::print("{}", details::format_to_local_buffer(ia, fmt, std::forward<Args>(args)...));
    } else {
        std::print(details::translate(ia, fmt.get()), std::forward<Args>(args)...);
    }
}

template<typename... Args>
void print(std::FILE* f, format_cstring<Args...> fmt, Args&&... args)
    pre (f != nullptr)
{
    std::print(f, details::translate({}, fmt.get()), std::forward<Args>(args)...);
}

template<typename... Args>
void print(std::FILE* f, const i18n_args& ia, format_cstring<Args...> fmt, Args&&... args)
    pre (f != nullptr)
{
    if (ia.locale) {
        std::print(f, "{}", details::format_to_local_buffer(ia, fmt, std::forward<Args>(args)...));
    } else {
        std::print(f, details::translate(ia, fmt.get()), std::forward<Args>(args)...);
    }
}

template<typename... Args>
void println(format_cstring<Args...> fmt, Args&&... args) {
    std::println(details::translate({}, fmt.get()), std::forward<Args>(args)...);
}

template<typename... Args>
void println(const i18n_args& ia, format_cstring<Args...> fmt, Args&&... args) {
    if (ia.locale) {
        std::println("{}", details::format_to_local_buffer(ia, fmt, std::forward<Args>(args)...));
    } else {
        std::println(details::translate(ia, fmt.get()), std::forward<Args>(args)...);
    }
}

template<typename... Args>
void println(std::FILE* f, format_cstring<Args...> fmt, Args&&... args)
    pre (f != nullptr)
{
    std::println(f, details::translate({}, fmt.get()), std::forward<Args>(args)...);
}

template<typename... Args>
void println(std::FILE* f, const i18n_args& ia, format_cstring<Args...> fmt, Args&&... args)
    pre (f != nullptr)
{
    if (ia.locale) {
        std::println(f, "{}", details::format_to_local_buffer(ia, fmt, std::forward<Args>(args)...));
    } else {
        std::println(f, details::translate(ia, fmt.get()), std::forward<Args>(args)...);
    }
}

template<typename... Args>
constexpr std::string plural_format(format_cstring<Args...> fmt,
                                    format_cstring<Args...> fmt_plural,
                                    Args&&... args) {
    return std::format(details::translate({}, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
}

template<typename... Args>
constexpr std::string plural_format(const i18n_args& ia,
                                    format_cstring<Args...> fmt,
                                    format_cstring<Args...> fmt_plural,
                                    Args&&... args) {
    if (ia.locale) {
        return std::format(*ia.locale, details::translate(ia, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
    } else {
        return std::format(details::translate(ia, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
    }
}

template<std::output_iterator<char> Out, typename... Args>
constexpr auto plural_format_to(Out out,
                                format_cstring<Args...> fmt,
                                format_cstring<Args...> fmt_plural,
                                Args&&... args) {
    return std::format_to(std::move(out), details::translate({}, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
}

template<std::output_iterator<char> Out, typename... Args>
constexpr auto plural_format_to(Out out,
                                const i18n_args& ia,
                                format_cstring<Args...> fmt,
                                format_cstring<Args...> fmt_plural,
                                Args&&... args) {
    if (ia.locale) {
        return std::format_to(std::move(out), *ia.locale, details::translate(ia, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
    } else {
        return std::format_to(std::move(out), details::translate(ia, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
    }
}

namespace details {

template<typename... Args>
inline std::string& plural_format_to_local_buffer(const i18n_args& ia,
                                           format_cstring<Args...> fmt,
                                           format_cstring<Args...> fmt_plural,
                                           Args&&... args)
    pre (ia.locale.has_value())
{
    std::string& buf = local_buffer();
    i18n::plural_format_to(std::back_inserter(buf), ia, fmt, fmt_plural, std::forward<Args>(args)...);
    return buf;
}

} // namespace clap::i18n::details

template<typename... Args>
void plural_print(format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    std::print(details::translate({}, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
}

template<typename... Args>
void plural_print(const i18n_args& ia,
                  format_cstring<Args...> fmt,
                  format_cstring<Args...> fmt_plural,
                  Args&&... args) {
    if (ia.locale) {
        std::print("{}", details::plural_format_to_local_buffer(ia, fmt, fmt_plural, std::forward<Args>(args)...));
    } else {
        std::print(details::translate(ia, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
    }
}

template<typename... Args>
void plural_print(std::FILE* f,
                  format_cstring<Args...> fmt,
                  format_cstring<Args...> fmt_plural,
                  Args&&... args)
    pre (f != nullptr)
{
    std::print(f, details::translate({}, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
}

template<typename... Args>
void plural_print(std::FILE* f,
                  const i18n_args& ia,
                  format_cstring<Args...> fmt,
                  format_cstring<Args...> fmt_plural,
                  Args&&... args)
    pre (f != nullptr)
{
    if (ia.locale) {
        std::print(f, "{}", details::plural_format_to_local_buffer(ia, fmt, fmt_plural, std::forward<Args>(args)...));
    } else {
        std::print(f, details::translate(ia, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
    }
}

template<typename... Args>
void plural_println(format_cstring<Args...> fmt, format_cstring<Args...> fmt_plural, Args&&... args) {
    std::println(details::translate({}, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
}

template<typename... Args>
void plural_println(const i18n_args& ia,
                    format_cstring<Args...> fmt,
                    format_cstring<Args...> fmt_plural,
                    Args&&... args) {
    if (ia.locale) {
        std::println("{}", details::plural_format_to_local_buffer(ia, fmt, fmt_plural, std::forward<Args>(args)...));
    } else {
        std::println(details::translate(ia, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
    }
}

template<typename... Args>
void plural_println(std::FILE* f,
                    format_cstring<Args...> fmt,
                    format_cstring<Args...> fmt_plural,
                    Args&&... args)
    pre (f != nullptr)
{
    std::println(f, details::translate({}, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
}

template<typename... Args>
void plural_println(std::FILE* f,
                    const i18n_args& ia,
                    format_cstring<Args...> fmt,
                    format_cstring<Args...> fmt_plural,
                    Args&&... args)
    pre (f != nullptr)
{
    if (ia.locale) {
        std::println(f, "{}", details::plural_format_to_local_buffer(ia, fmt, fmt_plural, std::forward<Args>(args)...));
    } else {
        std::println(f, details::translate(ia, fmt.get(), fmt_plural.get(), args...), std::forward<Args>(args)...);
    }
}

} // namespace clap::i18n

template<typename T>
struct std::formatter<clap::i18n::plural<T>, char> : std::formatter<T, char> {
    template <typename FormatContext>
    constexpr auto format(clap::i18n::plural<T> p, FormatContext& ctx) const {
        return std::formatter<T, char>::format(p.n, ctx);
    }
};

#endif // !CCCLAP_UTIL_I18N_H
