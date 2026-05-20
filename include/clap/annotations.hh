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
};

inline constexpr short_arg_annot short_arg = {};

struct long_arg_annot {
    std::string_view long_name;
    bool hidden = false;

    consteval static long_arg_annot operator()(std::string_view name, bool is_hidden = false) noexcept {
        return { .long_name = name, .hidden = is_hidden };
    }
};

inline constexpr long_arg_annot long_arg = {};

struct positional_annot {
    std::size_t pos = std::numeric_limits<std::size_t>::max();

    consteval static positional_annot operator()(std::size_t position) noexcept {
        return { .pos = position };
    }
};

inline constexpr positional_annot positional = {};

struct arg_annot {
    union arg_config {
        short_arg_annot short_arg;
        long_arg_annot long_arg;
        positional_annot positional;
    } config;
};

inline constexpr arg_annot arg = {};

} // namespace clap

#endif // !CCCLAP_ANNOTATIONS_H
