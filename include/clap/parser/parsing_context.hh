#pragma once
#ifndef CCCLAP_PARSER_PARSING_CONTEXT_H
#define CCCLAP_PARSER_PARSING_CONTEXT_H 1

#include <map>

#include <clap/parser/token.hh>
#include <clap/util/metax.hh>
#include <clap/util/cstring.hh>
#include <clap/util/any.hh>

namespace clap {

class parsing_context
{
public:
    using cursor = token_view::iterator;

    constexpr cursor current() const noexcept { return current_; }
    constexpr token_view argv() const noexcept { return args_; }

    constexpr cursor::difference_type absolute_position(cursor c) const noexcept { return c - current_; }
    constexpr cursor::difference_type relative_position(cursor c) const noexcept { return c - args_.begin(); }

    template<typename T, typename Self, typename Key>
        requires std::is_object_v<T>
    constexpr std::optional<metax::like_t<T, Self>> query_global_state(Self& self, const Key& key) noexcept {
        auto& states = self.custom_global_states_;
        if (auto it = states.find(key); it != states.end()) {
            return util::any_cast<metax::like_t<T, Self&>>(it->second);
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
    cursor current_ = {};
    std::map<std::string, util::any, std::less<>> custom_global_states_;
};

} // namespace clap

#endif // CCCLAP_PARSER_PARSING_CONTEXT_H
