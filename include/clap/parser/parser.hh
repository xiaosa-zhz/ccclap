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

#include <clap/parser/token.hh>
#include <clap/annotations.hh>
#include <clap/util/ascii.hh>
#include <clap/util/casecvt.hh>

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

struct NTCS_comparator {
    using is_transparent = void;

    [[nodiscard]] static constexpr bool operator()(const char* lhs, const char* rhs) noexcept {
        return std::ranges::lexicographical_compare(
            null_term(lhs), null_term(rhs));
    }

    template<std::convertible_to<std::string_view> V>
    [[nodiscard]] static constexpr bool operator()(const char* lhs, V&& rhs) noexcept {
        return std::ranges::lexicographical_compare(
            null_term(lhs), std::string_view(std::forward<V>(rhs)));
    }

    template<std::convertible_to<std::string_view> V>
    [[nodiscard]] static constexpr bool operator()(V&& lhs, const char* rhs) noexcept {
        return std::ranges::lexicographical_compare(
            std::string_view(std::forward<V>(lhs)), null_term(rhs));
    }

    template<std::convertible_to<std::string_view> V1, std::convertible_to<std::string_view> V2>
    [[nodiscard]] static constexpr bool operator()(V1&& lhs, V2&& rhs) noexcept {
        return std::string_view(std::forward<V1>(lhs)) < std::string_view(std::forward<V2>(rhs));
    }
};

template<typename Action, std::size_t N>
using lookup_table = std::flat_map<const char*, Action, NTCS_comparator,
    std::inplace_vector<const char*, N>, std::inplace_vector<Action, N>>;

template<typename Action>
using lookup_table_entry = lookup_table<Action, 0>::value_type;

template<typename Action, const lookup_table_entry<Action>* Table, std::size_t N>
constexpr lookup_table<Action, N> make_lookup_table() noexcept {
    return lookup_table<Action, N>(std::from_range, std::span(Table, N));
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

consteval std::vector<annotations::short_arg_annot> get_short_names(std::meta::info member) {
    std::vector<annotations::short_arg_annot> result;
    std::flat_map<char, annotations::short_arg_annot> exists;
    for (auto info : annotations_of(member)) {
        auto annot = short_arg;
        if (decay(type_of(info)) == ^^annotations::short_arg_annot) {
            annot = extract<annotations::short_arg_annot>(info);
        } else if (decay(type_of(info)) == ^^annotations::named_arg_annot) {
            annot = extract<annotations::named_arg_annot>(info).short_arg;
        } else {
            continue;
        }
        if (annot.from_member_name()) {
            // generete short name from member name
            std::string_view name = identifier_of(member);
            if (name.empty()) {
                throw std::meta::exception(
                    "short argument name cannot be generated from empty member name",
                    member);
            }
            // look for the first alphanumeric character in the member name to use as short name
            auto it = std::ranges::find_if(name, &ascii::is_alphanumeric<char>);
            if (it == name.end()) {
                throw std::meta::exception(
                    fmtext::format("short argument name cannot be generated from member name '{}' without alphanumeric characters", name),
                    member);
            }
            annot = annot(*it, annot.hidden);
        }
        if (auto it = exists.find(annot.short_name); it != exists.end()) {
            if ((*it).second != annot) {
                throw std::meta::exception(
                    fmtext::format("conflicting short argument configuration for member '{}': '{}'",
                        identifier_of(member), it->second.short_name),
                    member);
            }
        } else {
            exists.insert({ annot.short_name, annot });
            result.push_back(annot);
        }
    }
    return result;
}

consteval std::vector<annotations::long_arg_annot> get_long_names(std::meta::info member, style default_style) {
    std::vector<annotations::long_arg_annot> result;
    std::flat_map<std::string_view, annotations::long_arg_annot> exists;
    for (auto info : annotations_of(member)) {
        auto annot = long_arg;
        if (decay(type_of(info)) == ^^annotations::long_arg_annot) {
            annot = extract<annotations::long_arg_annot>(info);
        } else if (decay(type_of(info)) == ^^annotations::named_arg_annot) {
            annot = extract<annotations::named_arg_annot>(info).long_arg;
        } else {
            continue;
        }
        if (annot.from_member_name()) {
            // generete long name from member name
            std::string_view name = identifier_of(member);
            if (name.empty()) {
                throw std::meta::exception(
                    "long argument name cannot be generated from empty member name",
                    member);
            }
            // trim original name
            auto last_range = std::ranges::find_last_if(name, &ascii::is_alphanumeric<char>);
            if (last_range.empty()) {
                throw std::meta::exception(
                    fmtext::format("long argument name cannot be generated from member name '{}' without alphanumeric characters", name),
                    member);
            }
            auto first = std::ranges::find_if(name, &ascii::is_alphanumeric<char>);
            name = std::string_view(first, last_range.begin() + 1);
            if (annot.long_name_style == style::unspecified) {
                annot.long_name_style = default_style;
            }
            annot = annot(casecvt::convert(name, annot.long_name_style), annot.hidden);
        }
        if (auto it = exists.find(annot.long_name); it != exists.end()) {
            if ((*it).second != annot) {
                throw std::meta::exception(
                    fmtext::format("conflicting long argument configuration for member '{}': '{}'",
                        identifier_of(member), it->second.long_name),
                    member);
            }
        } else {
            exists.insert({ annot.long_name, annot });
            result.push_back(annot);
        }
    }
    return result;
}

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
