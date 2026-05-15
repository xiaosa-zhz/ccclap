#pragma once
#ifndef CCCLAP_LEX_LEXER_H
#define CCCLAP_LEX_LEXER_H 1

#include <string_view>
#include <charconv>
#include <array>
#include <algorithm>
#include <compare>
#include <concepts>
#include <iterator>
#include <ranges>

namespace clap {

enum class token_kind {
    eof,
    stdio,
    positional_escape,
    short_option,
    long_option,
    negative_number,
    argument,
    unknown,
};

constexpr bool is_stdio(std::string_view arg) noexcept {
    return arg == "-";
}

constexpr bool is_positional_escape(std::string_view arg) noexcept {
    return arg == "--";
}

constexpr bool is_short_option(std::string_view arg) noexcept {
    return arg.size() >= 2 && arg[0] == '-' && arg[1] != '-';
}

constexpr bool is_long_option(std::string_view arg) noexcept {
    return arg.size() >= 3 && arg.starts_with("--");
}

constexpr bool is_negative_number(std::string_view arg) noexcept {
    if (arg.size() < 2) return false;
    if (arg[0] != '-') return false;
    [[maybe_unused]] std::size_t res = 0;
    auto [_, ec] = std::from_chars(arg.data() + 1, arg.data() + arg.size(), res);
    return ec == std::errc();
}

constexpr void func(std::string_view arg) noexcept
    pre (!arg.empty())
{

}

struct token {
    std::string_view origin;
    std::string_view text;
};

class token_view : public std::ranges::view_interface<token_view>
{
public:
    token_view() = default;
    token_view(const token_view&) = default;
    token_view& operator=(const token_view&) = default;

    constexpr token_view(int argc, const char** argv, const char** original_argv) noexcept
        : original_argv(original_argv), argv(argv), argc(argc)
    {}

    class iterator
    {
    public:
        using iterator_concept = std::random_access_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = token;
        using difference_type = std::ptrdiff_t;
        using reference = value_type;

        constexpr iterator() noexcept = default;
        constexpr iterator(const iterator&) noexcept = default;
        constexpr iterator& operator=(const iterator&) noexcept = default;

        constexpr reference operator*() const noexcept {
            token res = { *original_argv, *argv };
            return res;
        }

        constexpr reference operator[](difference_type n) const noexcept {
            return *(*this + n);
        }

        constexpr iterator& operator++() noexcept {
            ++argv;
            ++original_argv;
            return *this;
        }

        constexpr iterator operator++(int) noexcept {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        constexpr iterator& operator--() noexcept {
            --argv;
            --original_argv;
            return *this;
        }

        constexpr iterator operator--(int) noexcept {
            auto copy = *this;
            --(*this);
            return copy;
        }

        constexpr iterator& operator+=(difference_type n) noexcept {
            argv += n;
            original_argv += n;
            return *this;
        }

        constexpr iterator& operator-=(difference_type n) noexcept {
            argv -= n;
            original_argv -= n;
            return *this;
        }

        friend constexpr iterator operator+(iterator it, difference_type n) noexcept {
            it += n;
            return it;
        }

        friend constexpr iterator operator+(difference_type n, iterator it) noexcept {
            it += n;
            return it;
        }

        friend constexpr iterator operator-(iterator it, difference_type n) noexcept {
            it -= n;
            return it;
        }

        friend constexpr difference_type operator-(const iterator& lhs, const iterator& rhs) noexcept {
            return lhs.argv - rhs.argv;
        }

        friend constexpr bool operator==(const iterator& lhs, const iterator& rhs) noexcept {
            return lhs.argv == rhs.argv
                && lhs.original_argv == rhs.original_argv;
        }

        friend constexpr std::strong_ordering operator<=>(const iterator& lhs, const iterator& rhs) noexcept {
            return lhs.argv <=> rhs.argv;
        }

    private:
        friend class token_view;
        constexpr iterator(const char** argv, const char** original_argv) noexcept
            : argv(argv), original_argv(original_argv)
        {}

        const char** argv = nullptr;
        const char** original_argv = nullptr;
    };

    constexpr iterator begin() noexcept {
        return iterator(argv, original_argv);
    }

    constexpr iterator begin() const noexcept {
        return iterator(argv, original_argv);
    }

    constexpr iterator end() noexcept {
        return iterator(argv + argc, original_argv + argc);
    }

    constexpr iterator end() const noexcept {
        return iterator(argv + argc, original_argv + argc);
    }

    constexpr std::size_t size() const noexcept {
        return static_cast<std::size_t>(argc);
    }

private:
    const char** original_argv = nullptr;
    const char** argv = nullptr;
    int argc = 0;
};

static_assert(std::random_access_iterator<token_view::iterator>);
static_assert(std::same_as<std::iter_reference_t<token_view::iterator>, token>);

} // namespace clap

#endif // CCCLAP_LEX_LEXER_H
