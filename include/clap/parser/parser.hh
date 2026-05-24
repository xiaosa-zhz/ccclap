#pragma once
#ifndef CCCLAP_PARSER_PARSER_GENERATOR_HH
#define CCCLAP_PARSER_PARSER_GENERATOR_HH 1

#include <concepts>
#include <meta>
#include <initializer_list>
#include <string_view>
#include <utility>
#include <inplace_vector>
#include <flat_map>
#include <algorithm>

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
// Consider using std::string_view when it is fixed.

struct null_sentinel_t {
    template<std::input_iterator I>
        requires std::default_initializable<std::iter_value_t<I>>
        && std::equality_comparable_with<std::iter_reference_t<I>, std::iter_value_t<I>>
    friend constexpr bool operator==(const I& it, null_sentinel_t) {
        return *it == std::iter_value_t<I>();
    }
};

inline constexpr null_sentinel_t null_sentinel;

struct null_term_fn {
    template<std::input_iterator I>
        requires std::default_initializable<std::iter_value_t<I>>
        && std::equality_comparable_with<std::iter_reference_t<I>, std::iter_value_t<I>>
    [[nodiscard]] constexpr auto operator()(I it) const {
        return std::ranges::subrange(std::move(it), null_sentinel);
    }
};

inline constexpr null_term_fn null_term = {};

struct null_terminated_string_comparator {
    using is_transparent = void;

    [[nodiscard]] constexpr bool operator()(const char* lhs, const char* rhs) const noexcept {
        return std::ranges::lexicographical_compare(
            null_term(lhs), null_term(rhs));
    }

    template<std::convertible_to<std::string_view> V>
    [[nodiscard]] constexpr bool operator()(const char* lhs, V&& view) const noexcept {
        return std::ranges::lexicographical_compare(
            null_term(lhs), std::string_view(std::forward<V>(view)));
    }

    template<std::convertible_to<std::string_view> V>
    [[nodiscard]] constexpr bool operator()(V&& view, const char* rhs) const noexcept {
        return std::ranges::lexicographical_compare(
            std::string_view(std::forward<V>(view)), null_term(rhs));
    }

    template<std::convertible_to<std::string_view> V1, std::convertible_to<std::string_view> V2>
    [[nodiscard]] constexpr bool operator()(V1&& v1, V2&& v2) const noexcept {
        return std::string_view(std::forward<V1>(v1)) < std::string_view(std::forward<V2>(v2));
    }
};

template<typename Action, std::size_t N>
using lookup_table = std::flat_map<const char*, Action, null_terminated_string_comparator,
    std::inplace_vector<const char*, N>, std::inplace_vector<Action, N>>;

template<typename Action>
using raw_lookup_table = std::pair<const char*, Action>;

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
