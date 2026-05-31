#pragma once

#include <concepts>
#include <meta>
#include <ranges>
#include <optional>
#include <functional>

#include <clap/util/cstring.hh>
#include <utility>

namespace clap::details {

template<typename T>
struct lazy_cell {
    template<typename F>
    struct type {
        F func;
        cstring_view str;

        operator T() const {
            return std::invoke_r<T>(func, str);
        }
    };
};

struct use_custom_parser_tag {};

template<typename T, auto Func>
struct custom_constant_parser {
    static constexpr T parse(cstring_view s) {
        return std::invoke_r<T>(Func, s);
    }
};

template<typename T, std::default_initializable Func>
struct custom_type_parser {
    static constexpr T parse(cstring_view s) {
        const Func parser;
        return std::invoke_r<T>(parser, s);
    }
};

template<typename NestedParser>
struct optional_parser {
    using type = typename NestedParser::type;

    static std::optional<type> parse(cstring_view s) {
        try {
            return std::optional<type>(std::in_place, NestedParser::parse(s));
        } catch (...) {
            return std::nullopt;
        }
    }
};

template<std::meta::reflection_range R = std::initializer_list<std::meta::info>>
consteval std::meta::info parser_dispatcher(std::meta::info member, R&& additional_inputs) {
    std::vector<std::meta::info> inputs(std::from_range, std::forward<R>(additional_inputs));
    auto type = type_of(member);
    if (inputs.size() >= 2) {
        // custom parser top priority
        auto first = inputs[0];
        if (is_type(first) && is_same_type(first, ^^use_custom_parser_tag)) {
            auto func = inputs[1];
            if (is_type(func)) {
                if (is_invocable_r_type(type, add_lvalue_reference(add_const(decay(func))), { ^^cstring_view })) {
                    return substitute(^^custom_type_parser, { type, func });
                }
            } else if (is_invocable_r_type(type, type_of(func), { ^^cstring_view })) {
                return substitute(^^custom_constant_parser, { type, func });
            }
        }
    }
    // container types
    if (is_specialization_of(type, ^^std::optional)) {
        auto nested = parser_dispatcher(member, inputs);
        if (nested) {
            return substitute(^^optional_parser, { nested });
        }
    }
}

} // namespace clap::details

/*

#pragma once
#ifndef CCCLAP_PARSER_ARGUMENT_ACTION_H
#define CCCLAP_PARSER_ARGUMENT_ACTION_H 1

#include <concepts>
#include <meta>
#include <initializer_list>
#include <charconv>
#include <system_error>

#include <clap/util/cstring.hh>

namespace clap {

namespace details {

// template<typename T>
// concept string_like = std::constructible_from<T, std::string_view>
//     && !std::same_as<T, std::string_view>
//     && !std::same_as<T, cstring_view>;

// template<typename T>
// concept cstring_like = std::constructible_from<T, const char*>
//     && !std::same_as<T, std::string_view>
//     && !std::same_as<T, cstring_view>;



} // namespace clap::details

// template<typename T, auto... Args>
// struct arg_val_parser {
//     static_assert(false, "Unsupported argument value parser");
// };

// template<typename T, auto Func>
//     requires (is_invocable_r_type(^^T, ^^decltype(Func), { ^^cstring_view }))
// struct arg_val_parser<T, ^^details::use_custom_parser_tag, Func> {
//     static constexpr T parse(cstring_view s) {
//         return Func(s);
//     }
// };

// template<>
// struct arg_val_parser<std::string_view> {
//     static constexpr std::string_view parse(cstring_view s) noexcept {
//         return s;
//     }
// };

// template<>
// struct arg_val_parser<cstring_view> {
//     static constexpr cstring_view parse(cstring_view s) noexcept {
//         return s;
//     }
// };

// template<details::string_like StringLike>
// struct arg_val_parser<StringLike> {
//     static constexpr StringLike parse(cstring_view s) {
//         return StringLike(std::string_view(s));
//     }
// };

// template<details::cstring_like CStringLike>
//     requires (!details::string_like<CStringLike>)
// struct arg_val_parser<CStringLike> {
//     static constexpr CStringLike parse(cstring_view s) {
//         return CStringLike(s.c_str());
//     }
// };

// template<typename T, auto FMT>
//     requires (is_arithmetic_type(^^T))
//     && (is_convertible_type(^^decltype(FMT), ^^int) || is_same_type(^^decltype(FMT), ^^std::chars_format))
// struct arg_val_parser<T, FMT> {
//     static constexpr T parse(cstring_view s) {
//         T value;
//         auto [_, ec] = std::from_chars(s.data(), s.data() + s.size(), value, FMT);
//         if (ec != std::errc()) {
//             throw std::system_error(std::make_error_code(ec));
//         }
//         return value;
//     }
// };

} // namespace clap

#endif // !CCCLAP_PARSER_ARGUMENT_ACTION_H

*/
