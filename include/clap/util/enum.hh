#pragma once
#ifndef CCCLAP_UTIL_ENUM_H
#define CCCLAP_UTIL_ENUM_H 1

#include <meta>
#include <optional>
#include <string_view>
#include <array>
#include <ranges>
#include <algorithm>

#include <clap/util/cstring.hh>

namespace clap {

template<typename Enum>
concept enum_type = is_enum_type(^^Enum);

namespace details {

template<typename Key, typename Value, std::size_t N>
using enum_string_lut = std::pair<std::array<Key, N>, std::array<Value, N>>;

template<enum_type Enum>
consteval auto enum_to_string_lut() {
    constexpr auto enumerators = std::define_static_array(enumerators_of(^^Enum));
    enum_string_lut<Enum, cstring_view, enumerators.size()> lut;
    for (auto&& [e, key, value] : std::views::zip(enumerators, lut.first, lut.second)) {
        auto name = identifier_of(e);
        key = extract<Enum>(e);
        value = { name.data(), name.size() };
    }
    std::ranges::sort(std::views::zip(lut.first, lut.second), {},
        [](auto&& entry) { return std::get<0>(entry); });
    return lut;
}

template<enum_type Enum>
consteval auto string_to_enum_lut() {
    constexpr auto enumerators = std::define_static_array(enumerators_of(^^Enum));
    enum_string_lut<cstring_view, Enum, enumerators.size()> lut;
    for (auto&& [e, key, value] : std::views::zip(enumerators, lut.first, lut.second)) {
        auto name = identifier_of(e);
        key = { name.data(), name.size() };
        value = extract<Enum>(e);
    }
    std::ranges::sort(std::views::zip(lut.first, lut.second), {},
        [](auto&& entry) { return std::get<0>(entry); });
    return lut;
}

} // namespace clap::details

template<enum_type Enum>
constexpr std::optional<cstring_view> enum_to_string(Enum value) noexcept {
    static constexpr auto lut = details::enum_to_string_lut<Enum>();
    auto it = std::ranges::lower_bound(lut.first, value);
    if (it != lut.first.end() && *it == value) {
        return lut.second[std::distance(lut.first.begin(), it)];
    }
    return std::nullopt;
}

template<enum_type Enum>
constexpr std::optional<Enum> string_to_enum(std::string_view str) noexcept {
    static constexpr auto lut = details::string_to_enum_lut<Enum>();
    auto it = std::ranges::lower_bound(lut.first, str);
    if (it != lut.first.end() && *it == str) {
        return lut.second[std::distance(lut.first.begin(), it)];
    }
    return std::nullopt;
}

} // namespace clap

#endif // !CCCLAP_UTIL_ENUM_H
