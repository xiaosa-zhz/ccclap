#pragma once
#ifndef CCCLAP_PARSER_PARSER_GENERATOR_H
#define CCCLAP_PARSER_PARSER_GENERATOR_H 1

#include <cstddef>
#include <concepts>
#include <meta>
#include <string_view>
#include <utility>
#include <inplace_vector>
#include <flat_map>
#include <algorithm>
#include <ranges>
#include <array>

#include <clap/parser/details/arg_annot_parser.hh>
#include <clap/parser/token.hh>
#include <clap/annotations.hh>
#include <clap/util/ascii.hh>
#include <clap/util/casecvt.hh>
#include <clap/util/cstring.hh>
#include <clap/util/metax.hh>
#include <clap/util/lookup_table.hh>

/*

struct foo {

    [[=arg]]
    int arg;

    [[=positional]]
    int val;

};

struct bar {

    [[=arg]]
    int arg;

};

struct program {

    [[=sub_command]]
    std::variant<foo, bar> cmds;

    [[=arg]]
    bool verbose = false;

};
*/

namespace clap {

namespace details {

template<typename Action, typename R>
constexpr util::lookup_table<cstring_view, Action> make_lookup_table(R&& entries) noexcept {
    std::flat_map<cstring_view, Action> map(std::from_range, std::forward<R>(entries));
    return { metax::define_static_string_array(map.keys()), std::define_static_array(map.values()) };
}

template<typename Action>
using short_name_map = std::array<Action, 128>;

template<typename CMD, typename ParentEnv = void>
struct command_env : ParentEnv {

};

template<typename CMD>
struct command_env<CMD, void> {
    static constexpr auto subcommand_lut = 0;
    static constexpr auto short_lut = 0;
    static constexpr auto long_lut = 0;
};

consteval std::meta::info find_subcommands(std::meta::info type) {
    std::meta::info subcommands_member = {};
    for (auto member : nonstatic_data_members_of(type, std::meta::access_context::current())) {
        auto type = decay(type_of(member));
        if (has_template_arguments(type)
            && template_of(type) == ^^std::variant
            && decay(template_arguments_of(type)[0]) == ^^annotations::subcommand_tag) {
            if (subcommands_member != std::meta::info{}) {
                throw std::meta::exception(
                    "multiple subcommands members found, only one is allowed",
                    type);
            }
            subcommands_member = member;
        }
    }
    return subcommands_member;
}

} // namespace clap::details

class parser
{
public:
    parser() = default;
    parser(const parser&) = default;
    parser& operator=(const parser&) = default;

    parser(token_view tokens) noexcept
        : cur(tokens.begin()), end(tokens.end())
    {}

    template<typename CMD>
    CMD parse() {
        CMD cmd;
        static constexpr bool enable_multicall = !annotations_of_with_type(^^CMD,
            ^^annotations::multicall_annot).empty();
        if constexpr (enable_multicall) {
            [] consteval {
                auto subcommands = details::find_subcommands(^^CMD);
                if (subcommands == std::meta::info{}) {
                    throw std::meta::exception("multicall command must have a subcommands member", ^^CMD);
                }
            }();
        }
        parse_command(cmd);
        return cmd;
    }

private:
    cstring_view to_next_token() noexcept {
        if (cur != end) {
            ++cur;
            unparsed_token = (*cur).text;
        } else {
            unparsed_token = {};
        }
        return unparsed_token;
    }

    template<typename Env>
    using action_type = void(*)(parser&, Env&);

    template<std::meta::info Arg, typename Env>
    void parse_argument(this parser& self, Env& env) {}

    template<typename Env>
    void parse_command(this parser& self, Env& env) {}

    token_view::iterator cur;
    token_view::iterator end;
    cstring_view unparsed_token;
};

// TODO
/*
constexpr bool is_stdio() const noexcept {
    return text == "-";
}

constexpr bool is_positional_escape() const noexcept {
    return text == "--";
}

constexpr bool is_short_option() const noexcept {
    return text.size() >= 2 && text[0] == '-' && text[1] != '-';
}

constexpr bool is_long_option() const noexcept {
    return text.size() >= 3 && text.starts_with("--");
}

constexpr bool is_negative_number() const noexcept {
    if (text.size() < 2) return false;
    if (text[0] != '-') return false;
    [[maybe_unused]] std::size_t res = 0;
    auto [_, ec] = std::from_chars(text.data() + 1, text.data() + text.size(), res);
    return ec == std::errc();
}
*/

} // namespace clap

#endif // !CCCLAP_PARSER_PARSER_GENERATOR_H
