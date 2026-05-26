#pragma once
#ifndef CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H
#define CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H 1

#include <meta>
#include <concepts>
#include <algorithm>
#include <ranges>

#include <clap/annotations.hh>
#include <clap/util/fmtext.hh>

namespace clap::details {

struct noop_parser {
    constexpr void do_parse(...) const noexcept {}
    constexpr void do_build() const noexcept {}
};

template<typename Annot>
inline constexpr std::meta::info find_argument_parser = ^^noop_parser;

struct arg_name_parser {
    constexpr void do_parse(annotations::long_arg_annot) const noexcept {}
    constexpr void do_parse(annotations::short_arg_annot) const noexcept {}
    constexpr void do_parse(annotations::named_arg_annot) const noexcept {}
    constexpr void do_build() const noexcept {}
};

template<typename Annot>
    requires std::same_as<Annot, annotations::long_arg_annot>
        || std::same_as<Annot, annotations::short_arg_annot>
        || std::same_as<Annot, annotations::named_arg_annot>
inline constexpr std::meta::info find_argument_parser<Annot> = ^^arg_name_parser;

struct help_parser {
    constexpr void do_parse(annotations::help_annot) const noexcept {}
    constexpr void do_build() const noexcept {}
};

template<>
inline constexpr std::meta::info find_argument_parser<annotations::help_annot> = ^^help_parser;

template<typename... Parsers>
struct all_argument_annotation_parser : Parsers... {
    using Parsers::do_parse...;
    using parse_helper_type = void(*)(all_argument_annotation_parser&);

    // FIXME: GCC has a bug that member function template (all kind of them)
    // could not have correct type when using meta::substitute on them.
    template<std::meta::info Annot>
    static constexpr parse_helper_type parse_helper = +[](all_argument_annotation_parser& parser) {
        parser.do_parse([:constant_of(Annot):]);
    };

    constexpr void parse(std::meta::info annot) {
        extract<parse_helper_type>(
            substitute(^^parse_helper, { reflect_constant(annot) })
        )(*this);
    }
};

using argument_annotation_parser = [:[] consteval {
    std::vector parser_types = members_of(^^annotations::argument_annotations, std::meta::access_context::current())
        | std::views::transform([](std::meta::info annot_type) {
            return extract<std::meta::info>(substitute(^^find_argument_parser, { annot_type }));
        })
        | std::ranges::to<std::vector>();
    std::ranges::sort(parser_types, [](auto a, auto b) { return type_order(a, b) < 0; });
    auto [first, last] = std::ranges::unique(parser_types);
    parser_types.erase(first, last);
    return substitute(^^all_argument_annotation_parser, parser_types);
}():];

} // namespace clap::details

#endif // !CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H
