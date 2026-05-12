#pragma once
#ifndef CCCLAP_LEX_LEXER_H
#define CCCLAP_LEX_LEXER_H 1

#include <string_view>
#include <charconv>

namespace clap {

constexpr bool is_stdio(std::string_view arg) noexcept {
    return arg == "-";
}

constexpr bool is_escape(std::string_view arg) noexcept {
    return arg == "--";
}

constexpr bool is_short_option(std::string_view arg) noexcept {
    return arg.size() >= 2 && arg[0] == '-' && arg[1] != '-';
}

constexpr bool is_long_option(std::string_view arg) noexcept {
    return arg.size() >= 3 && arg.starts_with("--");
}

constexpr bool is_negative_number(std::string_view arg) noexcept {
    if (arg.size() < 2) return false;
    if (arg[0] != '-') return false;
    std::size_t res;
    auto [_, ec] = std::from_chars(arg.data() + 1, arg.data() + arg.size(), res);
    return ec == std::errc();
}

} // namespace clap

#endif // CCCLAP_LEX_LEXER_H
