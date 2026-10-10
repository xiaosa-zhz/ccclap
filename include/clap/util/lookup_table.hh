#pragma once
#ifndef CCCLAP_UTIL_LOOKUP_TABLE_HH
#define CCCLAP_UTIL_LOOKUP_TABLE_HH 1

#include <span>
#include <optional>
#include <iterator>
#include <memory>
#include <algorithm>
#include <ranges>

namespace clap::util {

template<typename Key, typename Value, typename Compare = std::less<>>
class lookup_table
{
public:
    using key_type        = Key;
    using mapped_type     = Value;
    using value_type      = std::pair<const key_type, const mapped_type>;
    using key_compare     = Compare;
    using reference       = std::pair<const key_type&, const mapped_type&>;
    using const_reference = reference;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

private:
    class lookup_table_iterator
    {
    public:
        using difference_type = lookup_table::difference_type;
        using value_type      = lookup_table::value_type;
        using reference       = lookup_table::reference;
        using iterator_category = std::random_access_iterator_tag;

        class proxy_pointer
        {
        public:
            proxy_pointer() = delete;
            proxy_pointer(const proxy_pointer&) = delete;
            proxy_pointer& operator=(const proxy_pointer&) = delete;

            constexpr const reference* operator->() const noexcept {
                return std::addressof(ref);
            }

        private:
            friend lookup_table_iterator;
            explicit proxy_pointer(reference r) noexcept : ref(r) {}

            reference ref;
        };

        constexpr lookup_table_iterator() noexcept = default;
        constexpr lookup_table_iterator(const lookup_table_iterator&) noexcept = default;
        constexpr lookup_table_iterator& operator=(const lookup_table_iterator&) noexcept = default;

        constexpr reference operator*() const noexcept {
            return { *kptr_, *vptr_ };
        }

        constexpr proxy_pointer operator->() const noexcept {
            return proxy_pointer(**this);
        }

        constexpr lookup_table_iterator& operator++() noexcept {
            ++kptr_;
            ++vptr_;
            return *this;
        }

        constexpr lookup_table_iterator operator++(int) noexcept {
            lookup_table_iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        
        constexpr lookup_table_iterator& operator--() noexcept {
            --kptr_;
            --vptr_;
            return *this;
        }

        constexpr lookup_table_iterator operator--(int) noexcept {
            lookup_table_iterator tmp = *this;
            --(*this);
            return tmp;
        }

        constexpr lookup_table_iterator operator+(difference_type n) const noexcept {
            return lookup_table_iterator(kptr_ + n, vptr_ + n);
        }

        constexpr lookup_table_iterator& operator+=(difference_type n) noexcept {
            kptr_ += n;
            vptr_ += n;
            return *this;
        }

        friend constexpr lookup_table_iterator operator+(difference_type n, const lookup_table_iterator& it) noexcept {
            return it + n;
        }

        constexpr lookup_table_iterator operator-(difference_type n) const noexcept {
            return lookup_table_iterator(kptr_ - n, vptr_ - n);
        }

        constexpr lookup_table_iterator& operator-=(difference_type n) noexcept {
            kptr_ -= n;
            vptr_ -= n;
            return *this;
        }

        constexpr difference_type operator-(const lookup_table_iterator& other) const noexcept {
            return kptr_ - other.kptr_;
        }

        constexpr reference operator[](difference_type n) const noexcept {
            return *(*this + n);
        }

        friend constexpr auto operator<=>(const lookup_table_iterator& lhs, const lookup_table_iterator& rhs) noexcept {
            return lhs.kptr_ <=> rhs.kptr_;
        }

        friend constexpr bool operator==(const lookup_table_iterator& lhs, const lookup_table_iterator& rhs) noexcept {
            return lhs.kptr_ == rhs.kptr_;
        }

    private:
        friend lookup_table;
        explicit constexpr lookup_table_iterator(const key_type* kptr, const mapped_type* vptr) noexcept
            : kptr_(kptr), vptr_(vptr)
        {}

        const key_type*    kptr_ = nullptr;
        const mapped_type* vptr_ = nullptr;
    };

    static_assert(std::random_access_iterator<lookup_table_iterator>);

    template<typename TK>
    static constexpr bool transparent_key = requires (const key_compare& comp, const TK& tk, const key_type& k) {
        typename key_compare::is_transparent;
        { comp_(tk, k) }  -> std::convertible_to<bool>;
        { comp_(k, tk) }  -> std::convertible_to<bool>;
        { comp_(tk, tk) } -> std::convertible_to<bool>;
        { comp_(k, k) }   -> std::convertible_to<bool>;
    };

public:
    using iterator               = lookup_table_iterator;
    using const_iterator         = iterator;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using key_container_type     = std::span<const key_type>;
    using mapped_container_type  = std::span<const mapped_type>;

    constexpr lookup_table(key_container_type keys, mapped_container_type values, key_compare comp = {}) noexcept
        pre (std::ranges::size(keys) == std::ranges::size(values))
        pre (std::ranges::is_sorted(keys, comp))
        pre (std::ranges::adjacent_find(keys, std::not_fn(comp)) == keys.end())
        : comp_(comp), keys_(keys), values_(values)
    {}

    constexpr key_container_type    keys()   const noexcept { return keys_; }
    constexpr mapped_container_type values() const noexcept { return values_; }
    constexpr key_compare           comp() const noexcept { return comp_; }

    constexpr iterator begin()   const noexcept { return iterator(keys_.data(), values_.data()); }
    constexpr iterator end()     const noexcept { return iterator(keys_.data() + keys_.size(), values_.data() + values_.size()); }
    constexpr const_iterator cbegin() const noexcept { return begin(); }
    constexpr const_iterator cend()   const noexcept { return end(); }
    constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    constexpr reverse_iterator rend()   const noexcept { return reverse_iterator(begin()); }
    constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    constexpr const_reverse_iterator crend()   const noexcept { return rend(); }

    template<typename K>
        requires (not std::same_as<K, key_type>) && transparent_key<K>
    constexpr iterator find(const K& key) const noexcept {
        return find_impl(key);
    }

    constexpr iterator find(const key_type& key) const noexcept {
        return find_impl(key);
    }

    template<typename K>
        requires (not std::same_as<K, key_type>) && transparent_key<K>
    constexpr std::optional<const mapped_type&> lookup(const K& key) const noexcept {
        if (auto it = find(key); it != end()) {
            return it->second;
        }
        return std::nullopt;
    }

    constexpr std::optional<const mapped_type&> lookup(const key_type& key) const noexcept {
        if (auto it = find(key); it != end()) {
            return it->second;
        }
        return std::nullopt;
    }

    template<typename K>
        requires (not std::same_as<K, key_type>) && transparent_key<K>
    constexpr const mapped_type& at(const K& key) const {
        return *(lookup(key).or_else(&throw_out_of_range));
    }

    constexpr const mapped_type& at(const key_type& key) const {
        return *(lookup(key).or_else(&throw_out_of_range));
    }

private:
    template<typename K>
    constexpr iterator find_impl(const K& key) const noexcept {
        auto it = std::ranges::lower_bound(keys_, key, comp_);
        if (it != keys_.end() && !comp_(key, *it)) {
            return iterator(std::to_address(it), values_.data() + std::ranges::distance(keys_.begin(), it));
        }
        return end();
    }

    [[noreturn]] static constexpr std::optional<const mapped_type&> throw_out_of_range() {
        throw std::out_of_range("key not found");
    }

    [[no_unique_address]] key_compare comp_ = {};
    key_container_type    keys_;
    mapped_container_type values_;
};

} // namespace clap::util

#endif // CCCLAP_UTIL_LOOKUP_TABLE_HH
