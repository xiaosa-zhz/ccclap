#pragma once
#ifndef CCCLAP_UTIL_METAX_H
#define CCCLAP_UTIL_METAX_H 1

#include <meta>
#include <concepts>
#include <type_traits>
#include <span>
#include <vector>
#include <string_view>
#include <ranges>
#include <inplace_vector>

#include <clap/util/cstring.hh>

namespace clap::metax {

namespace details {

template<typename SVType, const auto*... Strs>
inline constexpr std::array<SVType, sizeof...(Strs)> sv_val = { Strs... };

template<typename T, const auto& Val>
inline constexpr std::pair<const T*, std::size_t> span_pair_val = { std::ranges::data(Val), std::ranges::size(Val) };

} // namespace clap::metax::details

consteval bool is_specialization_of(std::meta::info type, std::meta::info template_info) {
    return has_template_arguments(type) && template_of(type) == template_info;
}

// Convert range of string-like objects to a static string array (std::span<const cstring_view<CharT>>)
template<typename R>
    requires requires { typename std::ranges::range_value_t<std::ranges::range_value_t<R>>; }
consteval std::span<const std::ranges::range_value_t<R>> define_static_string_array(R&& r) {
    using char_type = std::ranges::range_value_t<std::ranges::range_value_t<R>>;
    using sv_type = basic_cstring_view<char_type>;
    static constexpr auto sv_info = ^^sv_type;
    std::vector<std::meta::info> args;
    args.push_back(sv_info);
    for (auto&& elem : std::forward<R>(r)) {
        args.push_back(std::meta::reflect_constant_string(elem));
    }
    const auto val_info = substitute(^^details::sv_val, args);
    const auto pair = extract<std::pair<const sv_type*, std::size_t>>(
        substitute(^^details::span_pair_val, { sv_info, val_info }));
    return { pair.first, pair.second };
}

} // namespace clap::metax

#endif // !CCCLAP_UTIL_METAX_H
