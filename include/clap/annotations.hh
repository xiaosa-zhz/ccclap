#pragma once
#ifndef CCCLAP_ANNOTATIONS_H
#define CCCLAP_ANNOTATIONS_H 1

#include <cstddef>
#include <meta>
#include <string_view>
#include <limits>
#include <source_location>
#include <variant>

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
        auto name_info = std::meta::reflect_constant_string(name);
        if (name.empty()) {
            throw std::meta::exception("long argument name cannot be empty",
                name_info, loc);
        }
        if (name.starts_with('-') || name.starts_with('_') || name.ends_with('-') || name.ends_with('_')) {
            throw std::meta::exception(
                "long argument name cannot start or end with hyphen or underscore",
                name_info, loc);
        }
        for (char c : name) {
            if (!(clap::ascii::is_alphanumeric(c) || c == '-' || c == '_')) {
                throw std::meta::exception(
                    "long argument name can only contains alphanumeric, hyphens, and/or underscores",
                    name_info, loc);
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
            sa = clap::short_arg(name[bo + 1], is_hidden, loc);
            std::string ln(name.substr(0, bo));
            ln += name.substr(bc + 1);
            la = clap::long_arg(ln, is_hidden, loc);
        } else {
            sa = clap::short_arg(name[0], is_hidden, loc);
            la = clap::long_arg(name, is_hidden, loc);
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
        sa = clap::short_arg(short_name, is_hidden, loc);
        la = clap::long_arg(long_style, is_hidden, loc);
        return result;
    }
};

// Generate both short and long named arguments.
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

struct shadow_parent_annot {};

// Mark a argument can shadow argument with the same name in parent command.
inline constexpr shadow_parent_annot shadow_parent{};

template<typename... CMDs>
using subcommands = std::variant<std::monostate, CMDs...>;

struct subcommand_name_annot {
    const char* name = nullptr;
    style command_name_style = style::kebab;

    static consteval subcommand_name_annot operator()(
            std::string_view name,
            std::source_location loc = std::source_location::current()) {
        auto name_info = std::meta::reflect_constant_string(name);
        if (name.empty()) {
            throw std::meta::exception("subcommand name cannot be empty", name_info, loc);
        }
        if (name.starts_with('-') || name.starts_with('_') || name.ends_with('-') || name.ends_with('_')) {
            throw std::meta::exception(
                "subcommand name cannot start or end with hyphen or underscore",
                name_info, loc);
        }
        for (char c : name) {
            if (!(clap::ascii::is_alphanumeric(c) || c == '-' || c == '_')) {
                throw std::meta::exception(
                    "subcommand name can only contains alphanumeric, hyphens, and/or underscores",
                    name_info, loc);
            }
        }
        return { .name = std::define_static_string(name), .command_name_style = style::custom };
    }

    static consteval subcommand_name_annot operator()(
            style s,
            std::source_location loc = std::source_location::current()) {
        if (s == style::custom) {
            throw std::meta::exception(
                "custom style cannot be specified directly", ^^style::custom, loc);
        }
        return { .name = nullptr, .command_name_style = s };
    }
};

// If subcommand type is not annotated with this annotation, it behaves like
// annotated with `[[=subcommand_name(style::kebab)]]` by default, which means
// the subcommand name is generated from the type name in kebab-case style
// (e.g., 'FooBar' -> 'foo-bar').
// Use this annotation to specify a custom subcommand name or change the naming style.
inline constexpr subcommand_name_annot sub_command = {};

struct mandate_subcommand_annot {};

// Mandate that one of the subcommands must be provided.
inline constexpr mandate_subcommand_annot mandate_subcommand{};

struct multicall_annot {};

// Makes program behave differently when called via different names (argv[0]).
// Invocation like `cmd1 ...` is same as `prog cmd1 ...`.
// Can only be applied to the root command, and it must have subcommands.
inline constexpr multicall_annot multicall{};

} // namespace clap

#endif // !CCCLAP_ANNOTATIONS_H
