#pragma once
#ifndef CCCLAP_LEX_LEXER_H
#define CCCLAP_LEX_LEXER_H 1

#include <concepts>
#include <string_view>
#include <charconv>
#include <compare>
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

// TODO:
//  - try_parse_* and these function needs to adapt to result value type

struct token {
    std::string_view text;
    std::string_view origin;

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

        constexpr reference operator*() const {
            contract_assert(*argv != nullptr && *original_argv != nullptr);
            return { .text = *argv, .origin = *original_argv };
        }

        constexpr reference operator[](difference_type n) const {
            return *(*this + n);
        }

        constexpr iterator& operator++()
            post (r : r.argv != nullptr)
            post (r : r.original_argv != nullptr)
        {
            ++argv;
            ++original_argv;
            return *this;
        }

        constexpr iterator operator++(int)
            post (r : r.argv != nullptr)
            post (r : r.original_argv != nullptr)
        {
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

        friend constexpr difference_type operator-(const iterator& lhs, const iterator& rhs)
            pre ((lhs.argv - rhs.argv) == (lhs.original_argv - rhs.original_argv))
        {
            return lhs.argv - rhs.argv;
        }

        friend constexpr bool operator==(const iterator& lhs, const iterator& rhs)
            pre ((lhs.argv == rhs.argv) == (lhs.original_argv == rhs.original_argv))
        {
            return lhs.argv == rhs.argv;
        }

        friend constexpr std::strong_ordering operator<=>(const iterator& lhs, const iterator& rhs)
            pre ((lhs.original_argv <=> rhs.original_argv) == (lhs.argv <=> rhs.argv))
        {
            return lhs.argv <=> rhs.argv;
        }

    private:
        friend class token_view;
        constexpr iterator(const char** argv, const char** original_argv)
            pre (argv != nullptr)
            pre (original_argv != nullptr)
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

#endif // CCCLAP_LEX_LEXER_H
