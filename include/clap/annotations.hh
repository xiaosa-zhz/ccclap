#pragma once
#ifndef CCCLAP_ANNOTATIONS_H
#define CCCLAP_ANNOTATIONS_H 1

#include <cstddef>
#include <string_view>
#include <limits>

namespace clap {

struct short_arg_annot {
    char short_name = '\0';
    bool hidden = false;

    consteval static short_arg_annot operator()(char name, bool is_hidden = false) noexcept {
        return { .short_name = name, .hidden = is_hidden };
    }

    constexpr bool from_member_name() const noexcept {
        return short_name == '\0';
    }
};

inline constexpr short_arg_annot short_arg = {};

struct long_arg_annot {
    std::string_view long_name;
    bool hidden = false;

    consteval static long_arg_annot operator()(std::string_view name, bool is_hidden = false) noexcept {
        return { .long_name = name, .hidden = is_hidden };
    }

    constexpr bool from_member_name() const noexcept {
        return long_name.empty();
    }
};

inline constexpr long_arg_annot long_arg = {};

struct positional_annot {
    std::size_t pos = std::numeric_limits<std::size_t>::max();

    consteval static positional_annot operator()(std::size_t position) noexcept {
        return { .pos = position };
    }

    constexpr bool from_member_position() const noexcept {
        return pos == std::numeric_limits<std::size_t>::max();
    }
};

inline constexpr positional_annot positional = {};

struct arg_annot {
    // TODO
    short_arg_annot short_arg;
    long_arg_annot long_arg;
    positional_annot positional;
};

inline constexpr arg_annot arg = {};

} // namespace clap

#endif // !CCCLAP_ANNOTATIONS_H
