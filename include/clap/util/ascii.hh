#pragma once
#ifndef CCCLAP_UTIL_ASCII_H
#define CCCLAP_UTIL_ASCII_H 1

#include <compare>

// ASCII character classification and manipulation utilities
// Copy from P3688

namespace clap::ascii {

template <typename T>
constexpr bool is_digit(T c, int base) noexcept
    pre (base >= 2 && base <= 36)
{
    auto v = static_cast<char32_t>(c);
    return (v >= U'0' && v < U'0' + (base < 10 ? base : 10))
        || (v >= U'a' && v < U'a' + (base > 10 ? base - 10 : 0))
        || (v >= U'A' && v < U'A' + (base > 10 ? base - 10 : 0));
}

template <typename T>
constexpr bool is_digit(T c) noexcept
{
    return is_digit(c, 10);
}

template <typename T>
constexpr bool is_hex_digit(T c) noexcept
{
    return is_digit(c, 16);
}

template <typename T>
constexpr bool is_octal_digit(T c) noexcept
{
    return is_digit(c, 8);
}

template <typename T>
constexpr bool is_bit(T c) noexcept
{
    return is_digit(c, 2);
}

template <typename T>
constexpr bool is_lower(T c) noexcept
{
    auto v = static_cast<char32_t>(c);
    return v >= U'a' && v <= U'z';
}

template <typename T>
constexpr bool is_upper(T c) noexcept
{
    auto v = static_cast<char32_t>(c);
    return v >= U'A' && v <= U'Z';
}

template <typename T>
constexpr bool is_alphabetic(T c) noexcept
{
    return is_lower(c) || is_upper(c);
}

template <typename T>
constexpr bool is_alphanumeric(T c) noexcept
{
    return is_alphabetic(c) || is_digit(c);
}

template <typename T>
constexpr bool is_whitespace(T c) noexcept
{
    auto v = static_cast<char32_t>(c);
    return v == U' '  || v == U'\f' || v == U'\n'
        || v == U'\r' || v == U'\t' || v == U'\v';
}

template <typename T>
constexpr bool is_horizontal_whitespace(T c) noexcept
{
    auto v = static_cast<char32_t>(c);
    return v == U' ' || v == U'\t';
}

template <typename T>
constexpr bool is_control(T c) noexcept
{
    auto v = static_cast<char32_t>(c);
    return v <= 0x1F || v == 0x7F;
}

template <typename T>
constexpr bool is_printing(T c) noexcept
{
    auto v = static_cast<char32_t>(c);
    return v >= U' ' && v <= U'~';
}

template <typename T>
constexpr bool is_punctuation(T c) noexcept
{
    auto v = static_cast<char32_t>(c);
    return (v >= U'!' && v <= U'/')
        || (v >= U':' && v <= U'@')
        || (v >= U'[' && v <= U'`')
        || (v >= U'{' && v <= U'~');
}

template <typename T>
constexpr T to_lower(T c) noexcept
{
    if (is_upper(c))
        return static_cast<T>(static_cast<char32_t>(c) - U'A' + U'a');
    return c;
}

template <typename T>
constexpr T to_upper(T c) noexcept
{
    if (is_lower(c))
        return static_cast<T>(static_cast<char32_t>(c) - U'a' + U'A');
    return c;
}

template <typename T>
constexpr std::strong_ordering case_insensitive_compare(T a, T b) noexcept
{
    return to_upper(a) <=> to_upper(b);
}

template <typename T>
constexpr bool case_insensitive_equals(T a, T b) noexcept
{
    return to_upper(a) == to_upper(b);
}

// digit value: '0'-'9' → 0-9, 'a'-'f'/'A'-'F' → 10-15, otherwise -1
template <typename T>
constexpr int digit_value(T c) noexcept
{
    auto v = static_cast<char32_t>(c);
    if (v >= U'0' && v <= U'9') return static_cast<int>(v - U'0');
    if (v >= U'a' && v <= U'f') return static_cast<int>(v - U'a' + 10);
    if (v >= U'A' && v <= U'F') return static_cast<int>(v - U'A' + 10);
    return -1;
}

} // namespace clap::ascii

#endif // CCCLAP_UTIL_ASCII_H
