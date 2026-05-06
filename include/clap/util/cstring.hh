#pragma once
#ifndef CCCLAP_UTIL_CSTRING_H
#define CCCLAP_UTIL_CSTRING_H 1

#include <cassert>
#include <cstddef>
#include <format>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <fmt/core.h>

// cstring_view: a string view that guarantees null-termination at data()[size()]
// Copy from P3655

namespace clap {

template <typename T>
concept cstring_like = requires(const T& t) {
    { t.c_str() } -> std::same_as<const typename T::value_type*>;
};

template <class CharT, class Traits = std::char_traits<CharT>>
class basic_cstring_view {
public:
    using traits_type               = Traits;
    using value_type                = CharT;
    using pointer                   = value_type*;
    using const_pointer             = const value_type*;
    using reference                 = value_type&;
    using const_reference           = const value_type&;
    using const_iterator            = const CharT*;
    using iterator                  = const_iterator;
    using const_reverse_iterator    = std::reverse_iterator<const_iterator>;
    using reverse_iterator          = const_reverse_iterator;
    using size_type                 = std::size_t;
    using difference_type           = std::ptrdiff_t;

    static constexpr size_type npos = size_type(-1);

private:
    static constexpr CharT empty_cstr[1]{};

public:
    constexpr basic_cstring_view() noexcept : size_(0) {
        data_ = std::data(empty_cstr);
    }

    constexpr basic_cstring_view(const basic_cstring_view&) noexcept            = default;
    constexpr basic_cstring_view& operator=(const basic_cstring_view&) noexcept = default;

    constexpr basic_cstring_view(const CharT* str)
        : basic_cstring_view(str, Traits::length(str)) {}

    constexpr basic_cstring_view(const CharT* str, size_type len)
        : data_(str), size_(len) {
        assert(str[len] == CharT());
    }

    constexpr basic_cstring_view(std::nullptr_t) = delete;

    constexpr basic_cstring_view(const cstring_like auto& r)
        : basic_cstring_view(r.c_str(), r.size()) {}

    constexpr const_iterator         begin()   const noexcept { return data_; }
    constexpr const_iterator         end()     const noexcept { return data_ + size_; }
    constexpr const_iterator         cbegin()  const noexcept { return begin(); }
    constexpr const_iterator         cend()    const noexcept { return end(); }
    constexpr const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator{end()}; }
    constexpr const_reverse_iterator rend()    const noexcept { return const_reverse_iterator{begin()}; }
    constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    constexpr const_reverse_iterator crend()   const noexcept { return rend(); }

    constexpr size_type size()     const noexcept { return size_; }
    constexpr size_type length()   const noexcept { return size_; }
    constexpr size_type max_size() const noexcept {
        return std::basic_string_view<CharT, Traits>{}.max_size() - 1;
    }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

    constexpr const_reference operator[](size_type pos) const {
        assert(pos <= size_);
        return data_[pos];
    }

    constexpr const_reference at(size_type pos) const {
        if (pos > size_) {
            throw std::out_of_range(
                std::format("basic_cstring_view::at: pos ({}) > size() {}", pos, size_));
        }
        return data_[pos];
    }

    constexpr const_reference front() const {
        assert(!empty());
        return data_[0];
    }

    constexpr const_reference back() const {
        assert(!empty());
        return data_[size_ - 1];
    }

    constexpr const_pointer data()  const noexcept { return data_; }
    constexpr const_pointer c_str() const noexcept { return data_; }

    constexpr operator std::basic_string_view<CharT, Traits>() const noexcept {
        return std::basic_string_view<CharT, Traits>{data_, size_};
    }

    constexpr void remove_prefix(size_type n) {
        assert(n <= size());
        data_ += n;
        size_ -= n;
    }

    constexpr void remove_suffix(size_type n) = delete(
        "cannot remove_suffix in-place on cstring_view while retaining "
        "null terminator. Use substr(pos, n) instead.");

    constexpr void swap(basic_cstring_view& s) noexcept {
        std::swap(data_, s.data_);
        std::swap(size_, s.size_);
    }

    constexpr size_type copy(CharT* s, size_type n, size_type pos = 0) const {
        return std::basic_string_view<CharT, Traits>(*this).copy(s, n, pos);
    }

    constexpr basic_cstring_view substr(size_type pos = 0) const {
        return basic_cstring_view{data_ + pos, size_ - pos};
    }
    constexpr basic_cstring_view subview(size_type pos = 0) const {
        return substr(pos);
    }

    constexpr std::basic_string_view<CharT, Traits> substr(size_type pos, size_type n) const {
        return std::basic_string_view<CharT, Traits>(*this).substr(pos, n);
    }
    constexpr std::basic_string_view<CharT, Traits> subview(size_type pos, size_type n) const {
        return substr(pos, n);
    }

    constexpr int compare(std::basic_string_view<CharT, Traits> s) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).compare(s);
    }
    constexpr int compare(size_type pos1, size_type n1, basic_cstring_view s) const {
        return std::basic_string_view<CharT, Traits>(*this).compare(pos1, n1, s);
    }
    constexpr int compare(size_type pos1, size_type n1, basic_cstring_view s,
                          size_type pos2, size_type n2) const {
        return std::basic_string_view<CharT, Traits>(*this).compare(pos1, n1, s, pos2, n2);
    }
    constexpr int compare(const CharT* s) const {
        return std::basic_string_view<CharT, Traits>(*this).compare(s);
    }
    constexpr int compare(size_type pos1, size_type n1, const CharT* s) const {
        return std::basic_string_view<CharT, Traits>(*this).compare(pos1, n1, s);
    }
    constexpr int compare(size_type pos1, size_type n1, const CharT* s, size_type n2) const {
        return std::basic_string_view<CharT, Traits>(*this).compare(pos1, n1, s, n2);
    }

    constexpr bool starts_with(std::basic_string_view<CharT, Traits> x) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).starts_with(x);
    }
    constexpr bool starts_with(CharT x) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).starts_with(x);
    }
    constexpr bool starts_with(const CharT* x) const {
        return std::basic_string_view<CharT, Traits>(*this).starts_with(x);
    }
    constexpr bool ends_with(std::basic_string_view<CharT, Traits> x) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).ends_with(x);
    }
    constexpr bool ends_with(CharT x) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).ends_with(x);
    }
    constexpr bool ends_with(const CharT* x) const {
        return std::basic_string_view<CharT, Traits>(*this).ends_with(x);
    }

    constexpr bool contains(std::basic_string_view<CharT, Traits> x) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).contains(x);
    }
    constexpr bool contains(CharT x) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).contains(x);
    }
    constexpr bool contains(const CharT* x) const {
        return std::basic_string_view<CharT, Traits>(*this).contains(x);
    }

    constexpr size_type find(std::basic_string_view<CharT, Traits> s, size_type pos = 0) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).find(s, pos);
    }
    constexpr size_type find(CharT c, size_type pos = 0) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).find(c, pos);
    }
    constexpr size_type find(const CharT* s, size_type pos, size_type n) const {
        return std::basic_string_view<CharT, Traits>(*this).find(s, pos, n);
    }
    constexpr size_type find(const CharT* s, size_type pos = 0) const {
        return std::basic_string_view<CharT, Traits>(*this).find(s, pos);
    }

    constexpr size_type rfind(std::basic_string_view<CharT, Traits> s, size_type pos = npos) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).rfind(s, pos);
    }
    constexpr size_type rfind(CharT c, size_type pos = npos) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).rfind(c, pos);
    }
    constexpr size_type rfind(const CharT* s, size_type pos, size_type n) const {
        return std::basic_string_view<CharT, Traits>(*this).rfind(s, pos, n);
    }
    constexpr size_type rfind(const CharT* s, size_type pos = npos) const {
        return std::basic_string_view<CharT, Traits>(*this).rfind(s, pos);
    }

    constexpr size_type find_first_of(std::basic_string_view<CharT, Traits> s, size_type pos = 0) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).find_first_of(s, pos);
    }
    constexpr size_type find_first_of(CharT c, size_type pos = 0) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).find_first_of(c, pos);
    }
    constexpr size_type find_first_of(const CharT* s, size_type pos, size_type n) const {
        return std::basic_string_view<CharT, Traits>(*this).find_first_of(s, pos, n);
    }
    constexpr size_type find_first_of(const CharT* s, size_type pos = 0) const {
        return std::basic_string_view<CharT, Traits>(*this).find_first_of(s, pos);
    }

    constexpr size_type find_last_of(std::basic_string_view<CharT, Traits> s, size_type pos = npos) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).find_last_of(s, pos);
    }
    constexpr size_type find_last_of(CharT c, size_type pos = npos) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).find_last_of(c, pos);
    }
    constexpr size_type find_last_of(const CharT* s, size_type pos, size_type n) const {
        return std::basic_string_view<CharT, Traits>(*this).find_last_of(s, pos, n);
    }
    constexpr size_type find_last_of(const CharT* s, size_type pos = npos) const {
        return std::basic_string_view<CharT, Traits>(*this).find_last_of(s, pos);
    }

    constexpr size_type find_first_not_of(std::basic_string_view<CharT, Traits> s, size_type pos = 0) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).find_first_not_of(s, pos);
    }
    constexpr size_type find_first_not_of(CharT c, size_type pos = 0) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).find_first_not_of(c, pos);
    }
    constexpr size_type find_first_not_of(const CharT* s, size_type pos, size_type n) const {
        return std::basic_string_view<CharT, Traits>(*this).find_first_not_of(s, pos, n);
    }
    constexpr size_type find_first_not_of(const CharT* s, size_type pos = 0) const {
        return std::basic_string_view<CharT, Traits>(*this).find_first_not_of(s, pos);
    }

    constexpr size_type find_last_not_of(std::basic_string_view<CharT, Traits> s,
                                         size_type pos = npos) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).find_last_not_of(s, pos);
    }
    constexpr size_type find_last_not_of(CharT c, size_type pos = npos) const noexcept {
        return std::basic_string_view<CharT, Traits>(*this).find_last_not_of(c, pos);
    }
    constexpr size_type find_last_not_of(const CharT* s, size_type pos, size_type n) const {
        return std::basic_string_view<CharT, Traits>(*this).find_last_not_of(s, pos, n);
    }
    constexpr size_type find_last_not_of(const CharT* s, size_type pos = npos) const {
        return std::basic_string_view<CharT, Traits>(*this).find_last_not_of(s, pos);
    }

    friend constexpr bool operator==(basic_cstring_view x, basic_cstring_view y) noexcept {
        return std::basic_string_view<CharT, Traits>(x) == std::basic_string_view<CharT, Traits>(y);
    }
    friend constexpr auto operator<=>(basic_cstring_view x, basic_cstring_view y) noexcept {
        return std::basic_string_view<CharT, Traits>(x) <=> std::basic_string_view<CharT, Traits>(y);
    }

private:
    const_pointer data_;
    size_type     size_;
};

template <class It, class End>
basic_cstring_view(It, End) -> basic_cstring_view<std::iter_value_t<It>>;

template <cstring_like R>
basic_cstring_view(R&&) -> basic_cstring_view<typename std::remove_cvref_t<R>::value_type>;

template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              basic_cstring_view<CharT, Traits>  str) {
    return os << std::basic_string_view<CharT, Traits>(str);
}

inline namespace literals {
inline namespace cstring_view_literals {

consteval basic_cstring_view<char> operator""_csv(const char* str, std::size_t len) noexcept {
    return basic_cstring_view<char>{str, len};
}
consteval basic_cstring_view<char8_t> operator""_csv(const char8_t* str, std::size_t len) noexcept {
    return basic_cstring_view<char8_t>{str, len};
}
consteval basic_cstring_view<char16_t> operator""_csv(const char16_t* str, std::size_t len) noexcept {
    return basic_cstring_view<char16_t>{str, len};
}
consteval basic_cstring_view<char32_t> operator""_csv(const char32_t* str, std::size_t len) noexcept {
    return basic_cstring_view<char32_t>{str, len};
}
consteval basic_cstring_view<wchar_t> operator""_csv(const wchar_t* str, std::size_t len) noexcept {
    return basic_cstring_view<wchar_t>{str, len};
}

} // namespace cstring_view_literals
} // namespace literals

using cstring_view    = basic_cstring_view<char>;
using u8cstring_view  = basic_cstring_view<char8_t>;
using u16cstring_view = basic_cstring_view<char16_t>;
using u32cstring_view = basic_cstring_view<char32_t>;
using wcstring_view   = basic_cstring_view<wchar_t>;

} // namespace clap

template <class CharT, class Traits>
struct std::formatter<clap::basic_cstring_view<CharT, Traits>, CharT>
    : std::formatter<std::basic_string_view<CharT, Traits>, CharT> {
    template <typename Out>
    auto format(const clap::basic_cstring_view<CharT, Traits>& csv,
                std::basic_format_context<Out, CharT>& ctx) const {
        return std::formatter<std::basic_string_view<CharT, Traits>, CharT>::format(
            std::basic_string_view<CharT, Traits>{csv.data(), csv.size()}, ctx);
    }
};

template <class CharT, class Traits>
struct fmt::formatter<clap::basic_cstring_view<CharT, Traits>, CharT>
    : fmt::formatter<fmt::basic_string_view<CharT>, CharT> {
    template <typename FormatContext>
    constexpr auto format(const clap::basic_cstring_view<CharT, Traits>& csv,
                              FormatContext& ctx) const {
        return fmt::formatter<fmt::basic_string_view<CharT>, CharT>::format(
            fmt::basic_string_view<CharT>(csv.data(), csv.size()), ctx);
    }
};

template <class CharT, class Traits>
struct std::hash<clap::basic_cstring_view<CharT, Traits>> {
    auto operator()(const clap::basic_cstring_view<CharT, Traits>& sv) const noexcept {
        return std::hash<std::basic_string_view<CharT, Traits>>{}(
            static_cast<std::basic_string_view<CharT, Traits>>(sv));
    }
};

#endif // !CCCLAP_UTIL_CSTRING_H
