#pragma once
#ifndef CCCLAP_LEX_LEXER_H
#define CCCLAP_LEX_LEXER_H 1

#include "clap/util/ascii.hh"

#include <string_view>

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

constexpr bool is_number(std::string_view s) noexcept {
    if (s.empty()) return false;
    bool has_point = false;
    bool has_digit = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (ascii::is_digit(c)) {
            has_digit = true;
        } else if (c == '.') {
            if (has_point) return false;
            has_point = true;
        } else {
            return false;
        }
    }
    return has_digit;
}

constexpr bool is_negative_number(std::string_view arg) noexcept {
    if (arg.size() < 2) return false;
    if (arg[0] != '-') return false;
    return is_number(arg.substr(1));
}

} // namespace clap

#endif // CCCLAP_LEX_LEXER_H
