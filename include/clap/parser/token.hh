#pragma once
#ifndef CCCLAP_PARSER_TOKEN_H
#define CCCLAP_PARSER_TOKEN_H 1

#include <concepts>
#include <compare>
#include <iterator>
#include <ranges>

#include <clap/util/cstring.hh>

namespace clap {

struct token {
    cstring_view text;
    cstring_view origin;
};

class token_view : public std::ranges::view_interface<token_view>
{
public:
    constexpr token_view() noexcept = default;
    constexpr token_view(const token_view&) noexcept = default;
    constexpr token_view& operator=(const token_view&) noexcept = default;

    constexpr token_view(int argc, const char** argv, const char** original_argv)
        pre (argc >= 0)
        pre (argv != nullptr)
        pre (original_argv != nullptr)
        : argv(argv), original_argv(original_argv), argc(argc)
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
            return { .text = *argv, .origin = *original_argv };
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
            return lhs.argv == rhs.argv;
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

    constexpr iterator begin() const noexcept {
        return iterator(argv, original_argv);
    }

    constexpr iterator end() const noexcept {
        return iterator(argv + argc, original_argv + argc);
    }

    constexpr std::size_t size() const noexcept {
        return static_cast<std::size_t>(argc);
    }

private:
    const char** argv = nullptr;
    const char** original_argv = nullptr;
    int argc = 0;
};

static_assert(std::random_access_iterator<token_view::iterator>);
static_assert(std::same_as<std::iter_reference_t<token_view::iterator>, token>);

} // namespace clap

#endif // !CCCLAP_PARSER_TOKEN_H
