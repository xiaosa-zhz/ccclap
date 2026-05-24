#pragma once
#ifndef CCCLAP_PARSER_PARSER_GENERATOR_HH
#define CCCLAP_PARSER_PARSER_GENERATOR_HH 1

#include <meta>
#include <initializer_list>
#include <string_view>
#include <utility>
#include <inplace_vector>
#include <flat_map>

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

// FIXME: GCC does not have trivial union yet, making std::inplace_vector
// not capable for non-trivial types during constant evaluation.

template<typename CharT, typename CharTraits = std::char_traits<CharT>>
struct basic_trivial_string_view {
    const CharT* data;
    std::size_t size;

    constexpr std::basic_string_view<CharT, CharTraits> to_string_view() const noexcept {
        return std::basic_string_view<CharT, CharTraits>(data, size);
    }

    constexpr operator std::basic_string_view<CharT, CharTraits>() const noexcept {
        return to_string_view();
    }

    friend constexpr auto operator<=>(
            const basic_trivial_string_view& lhs,
            const basic_trivial_string_view& rhs) noexcept {
        return lhs.to_string_view() <=> rhs.to_string_view();
    }

    friend constexpr auto operator<=>(
            const basic_trivial_string_view& lhs,
            std::basic_string_view<CharT, CharTraits> rhs) noexcept {
        return lhs.to_string_view() <=> rhs;
    }

    friend constexpr bool operator==(
            const basic_trivial_string_view& lhs,
            const basic_trivial_string_view& rhs) noexcept {
        return lhs.to_string_view() == rhs.to_string_view();
    }

    friend constexpr bool operator==(
            const basic_trivial_string_view& lhs,
            std::basic_string_view<CharT, CharTraits> rhs) noexcept {
        return lhs.to_string_view() == rhs;
    }
};

using trivial_string_view = basic_trivial_string_view<char>;

consteval trivial_string_view from_string_view(std::string_view sv) noexcept {
    return { std::define_static_string(sv), sv.size() };
}

template<typename Action, std::size_t N>
using lookup_table = std::flat_map<trivial_string_view, Action, std::less<>,
    std::inplace_vector<trivial_string_view, N>, std::inplace_vector<Action, N>>;

template<typename Action>
using raw_lookup_table = std::pair<trivial_string_view, Action>;

template<typename Action, const raw_lookup_table<Action>* Table, std::size_t N>
constexpr lookup_table<Action, N> make_lookup_table() noexcept {
    return lookup_table<Action, N>(std::from_range, std::span(Table, N));
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
    void parse(CMD& cmd) {
        constexpr static bool enable_multicall = !annotations_of_with_type(^^CMD,
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

    template<std::meta::info Arg, typename CMD>
    void parse_argument(this parser& self, CMD& cmd) {
        
    }

    template<typename CMD>
    void parse_command(CMD& cmd) {
        nonstatic_data_members_of(^^CMD, std::meta::access_context::current());
    }

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

#endif // !CCCLAP_PARSER_PARSER_GENERATOR_HH
