#pragma once
#ifndef CCCLAP_ANNOTATIONS_H
#define CCCLAP_ANNOTATIONS_H 1

#include <cstddef>
#include <meta>
#include <string_view>
#include <limits>
#include <source_location>

#include <clap/util/ascii.hh>
#include <clap/util/fmtext.hh>

namespace clap {

struct short_arg_annot {
    char short_name = '\0';
    bool hidden = false;

    static consteval short_arg_annot operator()(
            char name,
            bool is_hidden = false,
            std::source_location loc = std::source_location::current()) {
        if (!clap::ascii::is_alphanumeric(name)) {
            throw std::meta::exception(
                fmtext::format("short argument name must be alphanumeric, got '{}'(0x{:x})",
                    name, static_cast<std::size_t>(name)),
                std::meta::reflect_constant(name), loc);
        }
        return { .short_name = name, .hidden = is_hidden };
    }

    constexpr bool from_member_name() const noexcept {
        return short_name == '\0';
    }
};

// Generate short argument ('-x')
// Use the first character of the member name by default,
// or a custom character via `[[=short_arg('x')]]`.
inline constexpr short_arg_annot short_arg = {};

enum class style : std::uint8_t { custom, kebab, snake };

struct long_arg_annot {
    const char* long_name = nullptr;
    style long_name_style = style::kebab;
    bool hidden = false;

    static consteval long_arg_annot operator()(
            std::string_view name,
            bool is_hidden = false,
            std::source_location loc = std::source_location::current()) {
        if (name.empty()) {
            throw std::meta::exception("long argument name cannot be empty",
                std::meta::reflect_constant_string(name), loc);
        }
        for (char c : name) {
            if (!(clap::ascii::is_alphanumeric(c) || c == '-' || c == '_')) {
                throw std::meta::exception(
                    "long argument name can only contains alphanumeric, hyphens, and/or underscores",
                    std::meta::reflect_constant_string(name), loc);
            }
        }
        return { .long_name = std::define_static_string(name),
            .long_name_style = style::custom, .hidden = is_hidden };
    }

    static consteval long_arg_annot operator()(
            style s,
            bool is_hidden = false,
            std::source_location loc = std::source_location::current()) {
        if (s == style::custom) {
            throw std::meta::exception(
                "custom style cannot be specified directly", ^^style::custom, loc);
        }
        return { .long_name = nullptr, .long_name_style = s, .hidden = is_hidden };
    }

    constexpr bool from_member_name() const noexcept {
        return long_name == nullptr;
    }
};

// Generate long argument ('--foo-bar' or '--foo_bar').
// By default, the long name is generated from the member name
// in kebab-case style (e.g., 'fooBar' -> '--foo-bar').
// You can also specify a custom long name via `[[=long_arg("foo")]]`,
// or choose snake_case style via `[[=long_arg(style::snake)]]`.
// Explicitly specified name will not be modified.
inline constexpr long_arg_annot long_arg = {};

struct positional_annot {
    std::size_t pos = std::numeric_limits<std::size_t>::max();

    static consteval positional_annot operator()(std::size_t position) noexcept {
        return { .pos = position };
    }

    constexpr bool from_member_position() const noexcept {
        return pos == std::numeric_limits<std::size_t>::max();
    }
};

inline constexpr positional_annot positional = {};

struct named_arg_annot {
    short_arg_annot short_arg = {};
    long_arg_annot long_arg = {};

    // Generate both short and long arguments.  By default the short name is
    // the first character of the member name and the long name is the provided
    // string as-is.  Bracket notation "[X]" picks a custom short name:
    //   `[[=arg("foo[B]ar")]]` → '-B' and '--fooBar'
    static consteval named_arg_annot operator()(
            std::string_view name,
            bool is_hidden = false,
            std::source_location loc = std::source_location::current()) {
        named_arg_annot result;
        auto& [sa, la] = result;
        const auto bo = name.find('[');
        if (bo != std::string_view::npos) {
            const auto bc = name.find(']');
            if (bc == std::string_view::npos || bc != bo + 2) {
                throw std::meta::exception(fmtext::format("invalid bracket notation in '{}'", name),
                    std::meta::reflect_constant_string(name), loc);
            }
            sa = short_arg_annot{}(name[bo + 1], is_hidden, loc);
            std::string ln(name.substr(0, bo));
            ln += name.substr(bc + 1);
            la = long_arg_annot{}(ln, is_hidden, loc);
        } else {
            sa = short_arg_annot{}(name[0], is_hidden, loc);
            la = long_arg_annot{}(name, is_hidden, loc);
        }
        return result;
    }

    // Short from explicit char, long style from the second parameter.
    static consteval named_arg_annot operator()(
            char short_name,
            style long_style = style::kebab,
            bool is_hidden = false,
            std::source_location loc = std::source_location::current()) {
        named_arg_annot result;
        auto& [sa, la] = result;
        sa = short_arg_annot{}(short_name, is_hidden, loc);
        la = long_arg_annot{}(long_style, is_hidden, loc);
        return result;
    }
};

// Generate both short and long arguments.
// If more complex configuration is needed, short_arg and long_arg can be
// specified separately.
inline constexpr named_arg_annot arg = {};

struct help_annot {
    static constexpr char default_help[] = "No help text available yet.";
    const char* help_text = default_help;

    static consteval help_annot operator()(
            std::string_view text,
            std::source_location loc = std::source_location::current()) {
        if (text.empty()) {
            throw std::meta::exception("help text cannot be empty", ^^help_annot, loc);
        }
        return { .help_text = std::define_static_string(text) };
    }
};

// Generate help text for an argument.
// Example: `[[=help("Name of the person to greet")]]`.
inline constexpr help_annot help{};

} // namespace clap

#endif // !CCCLAP_ANNOTATIONS_H
