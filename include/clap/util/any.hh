#pragma once
#ifndef CCCLAP_UTIL_ANY_H
#define CCCLAP_UTIL_ANY_H 1

// std::any but constexpr compatible

#include <concepts>
#include <type_traits>
#include <typeinfo>
#include <initializer_list>
#include <utility>
#include <optional>
#include <memory>

#include <clap/util/metax.hh>

namespace clap::util {

class any
{
    struct metainfo {
        void (*deleter)(void*) noexcept = nullptr;
        void* (*copy)(const void*) = nullptr;
        std::optional<const std::type_info&> type = typeid(void);
    };

    template<typename T>
        requires std::is_copy_constructible_v<T>
    static constexpr metainfo info_of = {
        .deleter = +[](void* ptr) noexcept { delete static_cast<T*>(ptr); },
        .copy = +[](const void* ptr) -> void* { return new T(*static_cast<const T*>(ptr)); },
        .type = typeid(T)
    };

public:
    constexpr any() = default;

    constexpr any(const any& other)
        : data_(other.info_.copy(other.data_)), info_(other.info_)
    {}

    constexpr any(any&& other) noexcept :
        data_(std::exchange(other.data_, nullptr)),
        info_(std::exchange(other.info_, {}))
    {}

    template<typename T, typename... Args>
        requires std::is_copy_constructible_v<std::decay_t<T>>
              && std::constructible_from<std::decay_t<T>, Args...>
    explicit constexpr any(std::in_place_type_t<T>, Args&&... args)
        : data_(new T(std::forward<Args>(args)...)), info_(info_of<T>)
    {}

    template<typename T, typename U, typename... Args>
        requires std::is_copy_constructible_v<std::decay_t<T>>
              && std::constructible_from<std::decay_t<T>, std::initializer_list<U>, Args...>
    explicit constexpr any(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
        : data_(new T(il, std::forward<Args>(args)...)), info_(info_of<T>)
    {}

    template<typename T>
        requires std::is_copy_constructible_v<std::decay_t<T>>
              && (not std::same_as<std::decay_t<T>, any>)
              && (not metax::is_specialization_of(decay(^^T), ^^std::in_place_type_t))
    constexpr any(T&& value)
        : any(std::in_place_type<std::decay_t<T>>, std::forward<T>(value))
    {}

    constexpr any& operator=(const any& other) noexcept {
        any(other).swap(*this);
        return *this;
    }

    constexpr any& operator=(any&& other) noexcept {
        any(std::move(other)).swap(*this);
        return *this;
    }

    template<typename T>
        requires std::is_copy_constructible_v<std::decay_t<T>>
              && (not std::same_as<std::decay_t<T>, any>)
    constexpr any& operator=(T&& value) {
        any(std::forward<T>(value)).swap(*this);
        return *this;
    }

    constexpr ~any() { reset(); }

    constexpr void swap(any& other) noexcept {
        std::ranges::swap(data_, other.data_);
        std::ranges::swap(info_, other.info_);
    }

    friend constexpr void swap(any& lhs, any& rhs) noexcept { lhs.swap(rhs); }

    template<typename T, typename... Args>
        requires std::is_copy_constructible_v<std::decay_t<T>>
              && std::constructible_from<std::decay_t<T>, Args...>
    constexpr std::decay_t<T>& emplace(Args&&... args) {
        any(std::in_place_type<std::decay_t<T>>, std::forward<Args>(args)...).swap(*this);
        return *static_cast<std::decay_t<T>*>(data_);
    }

    template<typename T, typename U, typename... Args>
        requires std::is_copy_constructible_v<std::decay_t<T>>
              && std::constructible_from<std::decay_t<T>, std::initializer_list<U>, Args...>
    constexpr std::decay_t<T>& emplace(std::initializer_list<U> il, Args&&... args) {
        any(std::in_place_type<std::decay_t<T>>, il, std::forward<Args>(args)...).swap(*this);
        return *static_cast<std::decay_t<T>*>(data_);
    }

    constexpr void reset() noexcept {
        if (data_) {
            contract_assert(info_.deleter != nullptr);
            info_.deleter(data_);
            data_ = nullptr;
            info_ = metainfo{};
        }
    }

    constexpr bool has_value() const noexcept { return data_ != nullptr; }
    constexpr const std::type_info& type() const noexcept { return *info_.type; }

    template<typename T>
    friend constexpr T* any_cast(any* operand) noexcept;
    template<typename T>
    friend constexpr const T* any_cast(const any* operand) noexcept;
    template<typename T>
    friend constexpr T any_cast(const any& operand);
    template<typename T>
    friend constexpr T any_cast(any& operand);
    template<typename T>
    friend constexpr T any_cast(any&& operand);

private:
    void* data_ = nullptr;
    metainfo info_;
};

template<typename T>
constexpr T* any_cast(any* operand) noexcept {
    static_assert(!std::is_void_v<T>);
    if (!operand || operand->type() != typeid(T)) {
        return nullptr;
    }
    return static_cast<T*>(operand->data_);
}

template<typename T>
constexpr const T* any_cast(const any* operand) noexcept {
    static_assert(!std::is_void_v<T>);
    if (!operand || operand->type() != typeid(T)) {
        return nullptr;
    }
    return static_cast<const T*>(operand->data_);
}

template<typename T>
constexpr std::optional<T> any_cast(const any& operand)
    noexcept(std::is_nothrow_constructible_v<T, const std::remove_cvref_t<T>&>)
{
    using U = std::remove_cvref_t<T>;
    static_assert(std::is_constructible_v<T, const U&>);
    if (auto* ptr = any_cast<U>(std::addressof(operand))) {
        return *ptr;
    }
    return std::nullopt;
}

template<typename T>
constexpr std::optional<T> any_cast(any& operand)
    noexcept(std::is_nothrow_constructible_v<T, std::remove_cvref_t<T>&>)
{
    using U = std::remove_cvref_t<T>;
    static_assert(std::is_constructible_v<T, U&>);
    if (auto* ptr = any_cast<U>(std::addressof(operand))) {
        return *ptr;
    }
    return std::nullopt;
}

template<typename T>
constexpr std::optional<T> any_cast(any&& operand)
    noexcept(std::is_nothrow_constructible_v<T, std::remove_cvref_t<T>&&>)
{
    using U = std::remove_cvref_t<T>;
    static_assert(std::is_constructible_v<T, U&&>);
    if (auto* ptr = any_cast<U>(std::addressof(operand))) {
        return std::move(*ptr);
    }
    return std::nullopt;
}

} // namespace clap::util

#endif // CCCLAP_UTIL_ANY_H
