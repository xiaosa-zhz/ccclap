#pragma once
#ifndef CCCLAP_PARSER_DETAILS_ARGUMENT_VALUE_PARSER_H
#define CCCLAP_PARSER_DETAILS_ARGUMENT_VALUE_PARSER_H 1

#include <concepts>
#include <charconv>
#include <system_error>
#include <limits>

#include <clap/parser/parsing_context.hh>
#include <clap/util/metax.hh>
#include <clap/util/misc.hh>

// TODO: redesign arg parser by using a two steps parsing model:
// 1. (map) convert from cstring_view to a intermediate object target type
// 2. (reduce) collect all intermediate objects into final argument value

namespace clap::details {

using cursor = parsing_context::cursor;

template<typename T = void>
struct parse_result {
    T value;
    cursor next;
};

template<>
struct parse_result<void> {
    cursor next;
};

template<typename Mapper>
concept arg_val_mapper = requires (Mapper& p, parsing_context& ctx) {
    requires metax::is_specialization_of(decay(^^decltype(p.parse(ctx))), ^^parse_result);
    requires std::default_initializable<Mapper>;
    requires std::convertible_to<decltype(p.parse(ctx).value), typename Mapper::value_type>;
};

template<typename Reducer>
concept arg_val_reducer = requires (Reducer& a, parsing_context& ctx) {
    requires std::constructible_from<Reducer, typename Reducer::value_type&>;
    requires std::same_as<std::decay_t<decltype(a.parse(ctx))>, parse_result<void>>;
};

template<typename Op, typename T>
parse_result<> unwrap(parse_result<T> result, Op&& op) {
    std::forward<Op>(op)(std::move(result.value));
    return parse_result<>{ .next = result.next };
}

template<std::integral T>
class integral_mapper
{
public:
    using value_type = T;

    constexpr parse_result<T> parse(parsing_context& ctx) {
        parse_result<T> res{};
        cursor cur = ctx.current();
        const auto raw = cur->text;
        const auto begin = raw.data();
        const auto end = begin + raw.size();
        auto [ptr, ec] = std::from_chars(begin, end, res.value, base);
        if (ec != std::errc{}) {
            throw std::system_error(std::make_error_code(ec));
        }
        if (ptr != end) {
            throw std::system_error(std::make_error_code(std::errc::invalid_argument));
        }
        res.next = ++cur;
        return res;
    }

    constexpr void set_base(int b) noexcept { base = b; }

private:
    int base = 10;
};

template<std::floating_point T>
class floating_point_mapper
{
public:
    using value_type = T;

    constexpr parse_result<T> parse(parsing_context& ctx) {
        parse_result<T> res{};
        cursor cur = ctx.current();
        const auto raw = cur->text;
        const auto begin = raw.data();
        const auto end = begin + raw.size();
        auto [ptr, ec] = std::from_chars(begin, end, res.value);
        if (ec != std::errc{}) {
            throw std::system_error(std::make_error_code(ec));
        }
        if (ptr != end) {
            throw std::system_error(std::make_error_code(std::errc::invalid_argument));
        }
        res.next = ++cur;
        return res;
    }

    constexpr void set_format(std::chars_format fmt) noexcept { format = fmt; }

private:
    std::chars_format format = std::chars_format::general;
};

template<typename Mapper, typename ValueType = typename Mapper::value_type>
class assign_reducer : util::rooted
{
public:
    using mapper_type = Mapper;
    using mapper_value_type = mapper_type::value_type;
    using value_type = ValueType;

    static_assert(std::convertible_to<mapper_value_type, value_type>);

    explicit assign_reducer(value_type& value) : value(value) {}

    parse_result<> parse(parsing_context& ctx) {
        return unwrap(mapper.parse(ctx), [this](mapper_value_type&& v) {
            value = std::move(v);
        });
    }

private:
    value_type& value;
    mapper_type mapper;
};

template<typename Mapper, typename Container>
class append_reducer : util::rooted
{
public:
    using mapper_type = Mapper;
    using mapper_value_type = mapper_type::value_type;
    using value_type = Container;

    static_assert(requires (value_type& c, mapper_value_type&& v) { c.push_back(std::move(v)); });

    explicit append_reducer(value_type& value) : value(value) {}

    parse_result<> parse(parsing_context& ctx) {
        if (value.size() >= max_size) {
            throw std::system_error(std::make_error_code(std::errc::argument_list_too_long));
        }
        return unwrap(mapper.parse(ctx), [this](mapper_value_type&& v) {
            value.push_back(std::move(v));
        });
    }

private:
    using size_type = value_type::size_type;

    value_type& value;
    mapper_type mapper;
    size_type max_size = std::numeric_limits<size_type>::max();
};

// template<typename Mapper, typename Reducer>
// class arg_val_parser
// {
// public:
//     using mapper_type = Mapper;
//     using reducer_type = Reducer;
// private:
//     mapper_type mapper;
//     reducer_type reducer;
// };

// template<typename T>
// struct lazy_cell {
//     template<typename F>
//     struct type {
//         F func;
//         T get() const { return std::invoke_r<T>(func); }
//         operator T() const { return get(); }
//     };
// };

// consteval bool is_specialization_of(std::meta::info type, std::meta::info template_info) {
//     return has_template_arguments(type) && template_of(type) == template_info;
// }

// consteval bool is_std_optional(std::meta::info type) {
//     return is_specialization_of(type, ^^std::optional);
// }

// template<typename Container>
// concept eager_appendable = requires {
//     typename Container::value_type;
//     requires requires (Container c, typename Container::value_type v) {
//         c.push_back(std::move(v));
//     };
// };

// consteval bool is_eager_appendable_container(std::meta::info type) {
//     return extract<bool>(substitute(^^eager_appendable, { type }));
// }

// template<typename T>
// struct lazy_callable_dummy {
//     operator T() const {}
// };

// template<typename Container>
// concept lazy_appendable = requires {
//     typename Container::value_type;
//     requires requires (Container c, lazy_callable_dummy<typename Container::value_type> v) {
//         c.emplace_back(std::move(v));
//     };
// };

// consteval bool is_lazy_appendable_container(std::meta::info type) {
//     return extract<bool>(substitute(^^lazy_appendable, { type }));
// }

// template<typename Container>
// concept appendable = eager_appendable<Container> || lazy_appendable<Container>;

// consteval bool is_appendable_container(std::meta::info type) {
//     return extract<bool>(substitute(^^appendable, { type }));
// }

// struct use_custom_parser_tag {};

// template<typename T, auto Func>
// struct custom_constant_parser {
//     using type = T;
//     static constexpr T parse(cstring_view s) {
//         return std::invoke_r<T>(Func, s);
//     }
// };

// template<typename T, std::default_initializable Func>
// struct custom_type_parser {
//     using type = T;
//     static constexpr T parse(cstring_view s) {
//         const Func parser;
//         return std::invoke_r<T>(parser, s);
//     }
// };

// template<typename NestedParser>
// struct optional_parser {
//     using type = NestedParser::type;
//     static constexpr std::optional<type> parse(cstring_view s) {
//         return std::optional<type>(std::in_place, lazy_cell<type>::template type([s] {
//             return NestedParser::parse(s);
//         }));
//     }
// };

// template<std::meta::reflection_range R = std::initializer_list<std::meta::info>>
// consteval std::meta::info parser_dispatcher(std::meta::info type, R&& additional_inputs) {
//     std::vector<std::meta::info> inputs(std::from_range, std::forward<R>(additional_inputs));
//     if (inputs.size() >= 2) {
//         // custom parser top priority
//         auto first = inputs[0];
//         if (is_type(first) && is_same_type(first, ^^use_custom_parser_tag)) {
//             auto func = inputs[1];
//             if (is_type(func)) {
//                 if (is_invocable_r_type(type, add_lvalue_reference(add_const(decay(func))), { ^^cstring_view })) {
//                     return substitute(^^custom_type_parser, { type, func });
//                 }
//             } else if (is_invocable_r_type(type, type_of(func), { ^^cstring_view })) {
//                 return substitute(^^custom_constant_parser, { type, func });
//             }
//         }
//     }
//     // container types
//     if (is_specialization_of(type, ^^std::optional)) {
//         auto nested = parser_dispatcher(type, inputs);
//         return substitute(^^optional_parser, { nested });
//     } else if (is_appendable_container(type)) {
//         // TODO
//         return {};
//     }
//     return {}; // TODO
// }

// template<typename Target, typename RawInputs>
// consteval void parse_arg_val(Target& target, RawInputs&& raw_inputs) {

// }

} // namespace clap::details

#endif // !CCCLAP_PARSER_DETAILS_ARGUMENT_VALUE_PARSER_H
