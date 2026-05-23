#pragma once
#ifndef CCCLAP_UTIL_CASECVT_H
#define CCCLAP_UTIL_CASECVT_H 1

#include <concepts>
#include <cstdint>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <clap/util/ascii.hh>

namespace clap::casecvt {

enum class style : std::uint8_t {
    verbatim,
	kebab,
	snake,
	screaming_snake,
	camel,
	pascal,
};

using word_list = std::vector<std::string>;

namespace details {

constexpr bool is_separator(char ch) noexcept {
	return !ascii::is_alphanumeric(ch);
}

constexpr bool is_boundary(char prev, char curr, char next) noexcept {
	auto prev_is_digit = ascii::is_digit(prev);
	auto curr_is_digit = ascii::is_digit(curr);
	if (prev_is_digit != curr_is_digit) {
		return true;
	}
	if (ascii::is_lower(prev) && ascii::is_upper(curr)) {
		return true;
	}
	return ascii::is_upper(prev) && ascii::is_upper(curr) && ascii::is_lower(next);
}

constexpr void flush_word(word_list& words, std::string& current) {
	if (current.empty()) return;

	words.push_back(current);
	current.clear();
}

constexpr void append_lower(std::string& out, std::string_view word) {
	out.append_range(word | std::views::transform(&ascii::to_lower<char>));
}

constexpr void append_upper(std::string& out, std::string_view word) {
	out.append_range(word | std::views::transform(&ascii::to_upper<char>));
}

constexpr void append_capitalized(std::string& out, std::string_view word) {
	if (word.empty()) return;
	out.push_back(ascii::to_upper(word.front()));
    out.append_range(word.substr(1) | std::views::transform(&ascii::to_lower<char>));
}

constexpr void append_separator(std::string& out, char separator, bool& first_word) {
	if (!first_word) {
		out.push_back(separator);
	}
	first_word = false;
}

} // namespace clap::casecvt::details

[[nodiscard]]
constexpr auto split_words(std::string_view input) -> word_list {
	word_list words;
	std::string current;
	current.reserve(input.size());

	for (std::size_t index = 0; index < input.size(); ++index) {
		auto curr = input[index];
		if (details::is_separator(curr)) {
			details::flush_word(words, current);
			continue;
		}

		if (!current.empty()) {
			auto prev = input[index - 1];
			auto next = index + 1 < input.size() ? input[index + 1] : '\0';

			// Split acronym runs before the capital that starts a lowercase word.
			if (details::is_boundary(prev, curr, next)) {
				details::flush_word(words, current);
			}
		}

		current.push_back(ascii::to_lower(curr));
	}

	details::flush_word(words, current);
	return words;
}

template<std::ranges::input_range Range>
    requires std::constructible_from<std::string_view, std::ranges::range_value_t<Range>>
[[nodiscard]] constexpr auto join_words(const Range& words, style style) -> std::string {
	std::string result;

	if (style == style::kebab || style == style::snake
		|| style == style::screaming_snake) {
		bool first_word = true;
		auto separator = style == style::kebab ? '-' : '_';
		for (const auto& word : words) {
			auto word_view = std::string_view(word);
			if (word_view.empty()) {
				continue;
			}

			details::append_separator(result, separator, first_word);
			if (style == style::screaming_snake) {
				details::append_upper(result, word_view);
			} else {
				details::append_lower(result, word_view);
			}
		}
		return result;
	}

	bool first_word = true;
	for (const auto& word : words) {
		auto word_view = std::string_view(word);
		if (word_view.empty()) {
			continue;
		}

		if (style == style::camel && first_word) {
			details::append_lower(result, word_view);
		} else {
			details::append_capitalized(result, word_view);
		}
		first_word = false;
	}

	return result;
}

[[nodiscard]]
constexpr auto convert(std::string_view input, style style) -> std::string {
	return join_words(split_words(input), style);
}

[[nodiscard]]
constexpr auto to_kebab(std::string_view input) -> std::string {
	return convert(input, style::kebab);
}

[[nodiscard]]
constexpr auto to_snake(std::string_view input) -> std::string {
	return convert(input, style::snake);
}

[[nodiscard]]
constexpr auto to_screaming_snake(std::string_view input) -> std::string {
	return convert(input, style::screaming_snake);
}

[[nodiscard]]
constexpr auto to_camel(std::string_view input) -> std::string {
	return convert(input, style::camel);
}

[[nodiscard]]
constexpr auto to_pascal(std::string_view input) -> std::string {
	return convert(input, style::pascal);
}

} // namespace clap::casecvt

#endif // !CCCLAP_UTIL_CASECVT_H
