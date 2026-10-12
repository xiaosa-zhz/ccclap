#pragma once
#ifndef CCCLAP_PARSER_DETAILS_ARGUMENT_VALUE_PARSER_H
#define CCCLAP_PARSER_DETAILS_ARGUMENT_VALUE_PARSER_H 1

#include <meta>
#include <concepts>
#include <type_traits>
#include <initializer_list>
#include <utility>
#include <memory>
#include <ranges>
#include <optional>
#include <functional>
#include <map>

#include <clap/parser/token.hh>
#include <clap/util/cstring.hh>
#include <clap/util/any.hh>

// TODO: redesign arg parser by using a two steps parsing model:
// 1. (map) convert from cstring_view to a intermediate object target type
// 2. (reduce) collect all intermediate objects into final argument value

namespace clap::details {

class parsing_context
{
public:
    using iterator = token_view::iterator;

    constexpr iterator current() const noexcept { return current_; }
    constexpr token_view argv() const noexcept { return args_; }

    template<typename T, typename Key>
        requires std::is_object_v<T>
    constexpr std::optional<T&> query_global_state(const Key& key) {
        if (auto it = custom_global_states_.find(key); it != custom_global_states_.end()) {
            if (auto* ptr = util::any_cast<T>(std::addressof(it->second))) {
                return *ptr;
            }
        }
        return std::nullopt;
    }

    template<typename Key, typename T>
        requires std::constructible_from<std::string, Key>
    constexpr std::decay_t<T>& set_global_state(Key&& key, T&& value) {
        util::any& stored_state = custom_global_states_[std::forward<Key>(key)];
        return stored_state.emplace<std::decay_t<T>>(std::forward<T>(value));
    }

    template<typename Key, typename T, typename... Args>
        requires std::constructible_from<std::string, Key>
              && std::constructible_from<std::decay_t<T>, Args...>
    constexpr std::decay_t<T>& set_global_state(Key&& key, std::in_place_type_t<T>, Args&&... args) {
        util::any& stored_state = custom_global_states_[std::forward<Key>(key)];
        return stored_state.emplace<std::decay_t<T>>(std::forward<Args>(args)...);
    }

    template<typename Key, typename T, typename U, typename... Args>
        requires std::constructible_from<std::string, Key>
              && std::constructible_from<std::decay_t<T>, std::initializer_list<U>, Args...>
    constexpr std::decay_t<T>& set_global_state(Key&& key, std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args) {
        util::any& stored_state = custom_global_states_[std::forward<Key>(key)];
        return stored_state.emplace<std::decay_t<T>>(il, std::forward<Args>(args)...);
    }

private:
    token_view args_ = {};
    iterator current_ = {};
    std::map<std::string, util::any, std::less<>> custom_global_states_;
};

template<typename Parser>
concept arg_val_parser = requires (Parser& p, parsing_context& ctx) {
    { p.parse(ctx) } -> std::same_as<typename parsing_context::iterator>;
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
