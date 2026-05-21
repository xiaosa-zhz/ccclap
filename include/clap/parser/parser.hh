#pragma once
#ifndef CCCLAP_PARSER_PARSER_GENERATOR_HH
#define CCCLAP_PARSER_PARSER_GENERATOR_HH 1

#include <meta>
#include <initializer_list>

#include <clap/parser/token.hh>
#include <clap/annotations.hh>

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

consteval std::meta::info find_subcommands(std::meta::info type) {
    for (auto member : nonstatic_data_members_of(type, std::meta::access_context::current())) {
        auto type = dealias(type_of(member));
        if (has_template_arguments(type)
            && template_of(type) == ^^std::variant
            && dealias(template_arguments_of(type)[0]) == ^^std::monostate) {
            return member;
        }
    }
    return {};
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
    void parse_root_command(CMD& cmd) {
        constexpr static bool enable_multicall = !annotations_of_with_type(^^CMD, ^^multicall_annot).empty();
        if constexpr (enable_multicall) {
            [] consteval {
                auto subcommands = details::find_subcommands(^^CMD);
                if (subcommands == std::meta::info{}) {
                    throw std::meta::exception("multicall command must have a subcommands member", ^^CMD);
                }
            }();
        }
        parse_command(cmd);
    }

private:
    template<typename CMD>
    void parse_command(CMD& cmd) {
        nonstatic_data_members_of(^^CMD, std::meta::access_context::current());
    }

    token_view::iterator cur;
    token_view::iterator end;
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

#endif // !CCCLAP_PARSER_PARSER_GENERATOR_HH
