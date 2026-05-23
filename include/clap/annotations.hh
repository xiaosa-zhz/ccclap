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
#include <clap/util/casecvt.hh>

namespace clap {

using style = casecvt::style;

namespace annotations {

struct short_arg_annot {
    char short_name = '\0';
    bool hidden = false;

    static consteval short_arg_annot operator()(
            char name,
            bool is_hidden = false,
            std::source_location loc = std::source_location::current()) {
        if (!ascii::is_alphanumeric(name)) {
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

struct long_arg_annot {
    const char* long_name = nullptr;
    style long_name_style = style::unspecified;
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
            if (!(ascii::is_alphanumeric(c) || c == '-' || c == '_')) {
                throw std::meta::exception(
                    "long argument name can only contains alphanumeric, hyphens, and/or underscores",
                    name_info, loc);
            }
        }
        return {
            .long_name = extract<const char*>(name_info),
            .long_name_style = style::verbatim,
            .hidden = is_hidden
        };
    }

    static consteval long_arg_annot operator()(
            style s,
            bool is_hidden = false,
            std::source_location loc = std::source_location::current()) {
        if (s == style::verbatim) {
            throw std::meta::exception(
                "verbatim style cannot be specified directly", ^^style::verbatim, loc);
        }
        return { .long_name = nullptr, .long_name_style = s, .hidden = is_hidden };
    }

    constexpr bool from_member_name() const noexcept {
        return long_name == nullptr;
    }
};

struct positional_annot {
    std::size_t pos = std::numeric_limits<std::size_t>::max();

    static consteval positional_annot operator()(std::size_t position) noexcept {
        return { .pos = position };
    }

    constexpr bool from_member_position() const noexcept {
        return pos == std::numeric_limits<std::size_t>::max();
    }
};

struct named_arg_annot {
    short_arg_annot short_arg = {};
    long_arg_annot long_arg = {};

    static consteval named_arg_annot operator()(
            std::string_view name,
            char c = '\0',
            bool is_hidden = false,
            std::source_location loc = std::source_location::current()) {
        named_arg_annot result;
        auto& [sa, la] = result;
        la = long_arg_annot{}(name, is_hidden, loc);
        if (c == '\0') {
            c = name[0];
        }
        sa = short_arg_annot{}(c, is_hidden, loc);
        return result;
    }

    static consteval named_arg_annot operator()(
            char short_name,
            style long_style = style::unspecified,
            bool is_hidden = false,
            std::source_location loc = std::source_location::current()) {
        return {
            .short_arg = short_arg_annot{}(short_name, is_hidden, loc),
            .long_arg = long_arg_annot{}(long_style, is_hidden, loc)
        };
    }
};

struct arg_count_annot {
    std::size_t min = 0;
    std::size_t max = std::numeric_limits<std::size_t>::max();

    static consteval arg_count_annot operator()(std::size_t num) noexcept {
        return { .min = num, .max = num };
    }

    static consteval arg_count_annot operator()(
            std::size_t min, std::size_t max,
            std::source_location loc = std::source_location::current()) {
        if (min > max) {
            throw std::meta::exception(
                fmtext::format("min cannot be greater than max ({} > {})", min, max),
                ^^arg_count_annot, loc);
        }
        return { .min = min, .max = max };
    }
};

struct env_default_annot {
    const char* env_var = nullptr;
    style env_var_style = style::unspecified;

    static consteval env_default_annot operator()(
            std::string_view env_var_name,
            std::source_location loc = std::source_location::current()) {
        auto env_var_info = std::meta::reflect_constant_string(env_var_name);
        if (env_var_name.empty()) {
            throw std::meta::exception("environment variable name cannot be empty",
                env_var_info, loc);
        }
        for (char c : env_var_name) {
            if (!(ascii::is_alphanumeric(c) || c == '_')) {
                throw std::meta::exception(
                    "environment variable name can only contains alphanumeric characters and underscores",
                    env_var_info, loc);
            }
        }
        return { .env_var = extract<const char*>(env_var_info), .env_var_style = style::verbatim };
    }

    static consteval env_default_annot operator()(
            style s,
            std::source_location loc = std::source_location::current()) {
        if (s == style::verbatim) {
            throw std::meta::exception(
                "verbatim style cannot be specified directly", ^^style::verbatim, loc);
        }
        return { .env_var = nullptr, .env_var_style = s };
    }
};

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

struct propagated_annot {};

struct shadow_parent_annot {};

struct subcommand_name_annot {
    const char* name = nullptr;
    style command_name_style = style::unspecified;

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
            if (!(ascii::is_alphanumeric(c) || c == '-' || c == '_')) {
                throw std::meta::exception(
                    "subcommand name can only contains alphanumeric, hyphens, and/or underscores",
                    name_info, loc);
            }
        }
        return { .name = std::define_static_string(name), .command_name_style = style::verbatim };
    }

    static consteval subcommand_name_annot operator()(
            style s,
            std::source_location loc = std::source_location::current()) {
        if (s == style::verbatim) {
            throw std::meta::exception(
                "verbatim style cannot be specified directly", ^^style::verbatim, loc);
        }
        return { .name = nullptr, .command_name_style = s };
    }
};

struct mandate_subcommand_annot {};

struct multicall_annot {};

struct arg_naming_style_annot {
    style naming_style = style::kebab;

    static consteval arg_naming_style_annot operator()(style s) noexcept {
        return { .naming_style = s };
    }
};

struct env_var_naming_style_annot {
    style naming_style = style::screaming_snake;

    static consteval env_var_naming_style_annot operator()(style s) noexcept {
        return { .naming_style = s };
    }
};

} // namespace annotations

template<typename... CMDs>
using subcommands = std::variant<std::monostate, CMDs...>;

// Generate short argument ('-x')
// Use the first character of the member name by default,
// or a custom character via `[[=short_arg('x')]]`.
inline constexpr annotations::short_arg_annot short_arg = {};

// Generate long argument ('--foo-bar' or '--foo_bar').
// By default, the long name is generated from the member name
// in kebab-case style (e.g., 'fooBar' -> '--foo-bar').
// You can also specify a custom long name via `[[=long_arg("foo")]]`,
// or choose snake_case style via `[[=long_arg(style::snake)]]`.
// Explicitly specified name will not be modified.
inline constexpr annotations::long_arg_annot long_arg = {};

inline constexpr annotations::positional_annot positional = {};

// Generate both short and long named arguments.
// If more complex configuration is needed, short_arg and long_arg can be
// specified separately.
inline constexpr annotations::named_arg_annot arg = {};

// Specify the number of values an argument can take.
inline constexpr annotations::arg_count_annot count{};

// Specify that the default value of an argument comes from an environment variable
// if no value is provided in the command line.
// Can be used together with normal default value, and the normal default value
// will be used if both environment variable and command line value are not provided.
// By default, the environment variable name is generated from the argument name,
// and naming style is determined by the scope setting (which is SCREAMING_SNAKE_CASE
// by default for env vars, but can be changed using [[=env_var_style(...)]] at command level).
// (e.g., 'fooBar' -> 'FOO_BAR').
// You can also specify a custom environment variable name via `[[=env_default("FOO")]]`,
// or choose a different naming style via `[[=env_default(style::snake)]]`.
inline constexpr annotations::env_default_annot env{};

// Generate help text for an argument.
// Example: `[[=help("Name of the person to greet")]]`.
inline constexpr annotations::help_annot help{};

// Mark an argument to be propagated to subcommands, so that it can still be
// parsed and used when parsing subcommands. If subcommand has an argument
// with the same name, an error will be emitted unless the subcommand argument
// is annotated with `[[=shadow_parent]]` to explicitly allow shadowing.
inline constexpr annotations::propagated_annot propagated{};

// Mark a argument can shadow argument with the same name in parent command.
inline constexpr annotations::shadow_parent_annot shadow_parent{};

// If subcommand type is not annotated with this annotation, it behaves like
// annotated with `[[=subcommand_name(style::unspecified)]]` by default, which means
// the subcommand name is generated from the type name, and the naming style
// is determined by the scope setting (which is kebab-case by default,
// but can be changed using [[=arg_style(...)]] at command level).
// (e.g., 'FooBar' -> 'foo-bar').
// Use this annotation to specify a custom subcommand name or change the naming style.
inline constexpr annotations::subcommand_name_annot sub_command = {};

// Mandate that one of the subcommands must be provided.
inline constexpr annotations::mandate_subcommand_annot mandate_subcommand{};

// Makes program behave differently when called via different names (argv[0]).
// Invocation like `cmd1 ...` is same as `prog cmd1 ...`.
// Can only be applied to the root command, and it must have subcommands.
inline constexpr annotations::multicall_annot multicall{};

// Specify the naming style for arguments generated from member names.
// By default, it is kebab-case (e.g., 'fooBar' -> '--foo-bar'),
// but it can be changed to snake_case, camelCase, etc.
// using this annotation at the command level.
inline constexpr annotations::arg_naming_style_annot arg_style = {};

// Specify the naming style for environment variables generated from argument names.
// By default, it is SCREAMING_SNAKE_CASE (e.g., 'fooBar' -> 'FOO_BAR'),
// but it can be changed to snake_case, camelCase, etc.
// using this annotation at the command level.
inline constexpr annotations::env_var_naming_style_annot env_var_style = {};

} // namespace clap

#endif // !CCCLAP_ANNOTATIONS_H
