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

template<typename T>
struct reflect_constant_customization_fn {
    static consteval std::meta::info operator()(const T& val) {
        return std::meta::reflect_constant(val);
    }
};

template<typename T>
inline constexpr reflect_constant_customization_fn<T> reflect_constant_customization = {};

// Additional reflect_constant support by this lib

template<typename K, typename V>
struct reflect_constant_customization_fn<std::pair<K, V>> {
    template<std::meta::info KeyVal, std::meta::info ValueVal>
    static constexpr auto pair_val = std::pair<K, V>([:KeyVal:], [:ValueVal:]);

    static consteval std::meta::info operator()(const std::pair<K, V>& p) {
        return substitute(^^pair_val, {
            std::meta::reflect_constant(reflect_constant_customization<std::remove_cv_t<K>>(p.first)),
            std::meta::reflect_constant(reflect_constant_customization<std::remove_cv_t<V>>(p.second)),
        });
    }
};

template<typename CharT>
struct reflect_constant_customization_fn<std::basic_string_view<CharT>> {
    template<const CharT* Ptr, std::size_t N>
    static constexpr auto string_view_val = std::basic_string_view<CharT>(Ptr, N);

    static consteval std::meta::info operator()(std::basic_string_view<CharT> sv) {
        return substitute(^^string_view_val, {
            std::meta::reflect_constant(sv.data()),
            std::meta::reflect_constant(sv.size()),
        });
    }
};

template<typename CharT>
struct reflect_constant_customization_fn<basic_cstring_view<CharT>> {
    template<const CharT* Ptr, std::size_t N>
    static constexpr auto cstring_view_val = basic_cstring_view<CharT>(Ptr, N);

    static consteval std::meta::info operator()(basic_cstring_view<CharT> csv) {
        return substitute(^^cstring_view_val, {
            std::meta::reflect_constant(csv.data()),
            std::meta::reflect_constant(csv.size()),
        });
    }
};

template<typename T, std::size_t Ext>
struct reflect_constant_customization_fn<std::span<const T, Ext>> {
    template<const T* Ptr, std::size_t N>
    static constexpr auto span_val = std::span<const T, Ext>(Ptr, N);

    static consteval std::meta::info operator()(std::span<const T, Ext> sp) {
        return substitute(^^span_val, {
            std::meta::reflect_constant(sp.data()),
            std::meta::reflect_constant(sp.size()),
        });
    }
};

template<typename T, std::size_t N>
struct reflect_constant_customization_fn<std::inplace_vector<T, N>> {
    template<std::meta::info... Vals>
    static constexpr auto inplace_vector_val = [] consteval {
        static_assert(sizeof...(Vals) <= N, "Too many values for inplace_vector");
        std::inplace_vector<T, N> vec;
        vec.append_range(std::to_array({ [:Vals:]... }));
        return vec;
    }();

    static consteval std::meta::info operator()(const std::inplace_vector<T, N>& vec) {
        std::vector<std::meta::info> args;
        for (auto& val : vec) {
            args.push_back(std::meta::reflect_constant(reflect_constant_customization<std::remove_cv_t<T>>(val)));
        }
        return substitute(^^inplace_vector_val, args);
    }
};

template<typename K, typename V, typename Comp, typename KC, typename VC>
struct reflect_constant_customization_fn<std::flat_map<K, V, Comp, KC, VC>> {
    template<std::meta::info KCVal, std::meta::info VCVal>
    static constexpr auto flat_map_val = std::flat_map<K, V, Comp, KC, VC>(std::sorted_unique, [:KCVal:], [:VCVal:]);

    static consteval std::meta::info operator()(const std::flat_map<K, V, Comp, KC, VC>& map) {
        return substitute(^^flat_map_val, {
            std::meta::reflect_constant(reflect_constant_customization<KC>(map.keys())),
            std::meta::reflect_constant(reflect_constant_customization<VC>(map.values())),
        });
    }

};

// Niebloids

struct reflect_constant_fn {
    template<typename T>
    static consteval std::meta::info operator()(T&& val) {
        return reflect_constant_customization<std::remove_cvref_t<T>>(val);
    }
};

struct define_static_object_fn {
    template<typename T>
    static consteval const std::remove_cvref_t<T>* operator()(T&& val) {
        const auto info = reflect_constant_fn{}(std::forward<T>(val));
        return std::addressof(extract<const std::remove_cvref_t<T>&>(info));
    }
};

template<typename T, std::meta::info... Vals>
inline constexpr T static_array[] = { [:Vals:]... };

template<typename T>
inline constexpr std::array<T, 0> empty_array;

struct reflect_constant_array_fn {
    template<std::ranges::input_range R>
    static consteval std::meta::info operator()(R&& r) {
        std::vector<std::meta::info> args;
        args.push_back(^^std::ranges::range_value_t<R>);
        for (auto&& val : std::forward<R>(r)) {
            args.push_back(std::meta::reflect_constant(reflect_constant_fn{}(decltype(val)(val))));
        }
        if (args.size() == 1) {
            return substitute(^^empty_array, args);
        } else {
            return substitute(^^static_array, args);
        }
    }
};

template<std::meta::info Underlying>
inline constexpr const auto* static_array_ptr = std::ranges::data([:Underlying:]);

struct define_static_array_fn {
    template<std::ranges::input_range R>
    static consteval std::span<const std::ranges::range_value_t<R>> operator()(R&& r) {
        const auto info = reflect_constant_array_fn{}(std::forward<R>(r));
        return {
            extract<const std::ranges::range_value_t<R>*>(substitute(^^static_array_ptr, { std::meta::reflect_constant(info) })),
            std::ranges::size(std::forward<R>(r)),
        };
    }
};

} // namespace details

inline constexpr details::reflect_constant_fn reflect_constant = {};
inline constexpr details::define_static_object_fn define_static_object = {};
inline constexpr details::reflect_constant_array_fn reflect_constant_array = {};
inline constexpr details::define_static_array_fn define_static_array = {};

} // namespace clap::metax

#endif // !CCCLAP_UTIL_METAX_H
