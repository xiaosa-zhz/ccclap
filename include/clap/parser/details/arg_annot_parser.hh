#pragma once
#ifndef CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H
#define CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H 1

#include <meta>
#include <concepts>

#include <clap/annotations.hh>

namespace clap::details {

template<typename Annot>
struct argument_annotation_parser {
    constexpr void do_parse(Annot annot) const noexcept {}
    constexpr void do_build() const noexcept {}
};

template<typename Annot>
    requires std::same_as<Annot, annotations::long_arg_annot>
    || std::same_as<Annot, annotations::short_arg_annot>
    || std::same_as<Annot, annotations::named_arg_annot>
struct argument_annotation_parser<Annot> {

};

template<>
struct argument_annotation_parser<annotations::help_annot> {

};

template<typename... Parsers>
struct all_argument_annotation_parser : Parsers... {

    template<std::meta::info Annot>
    constexpr void parse_helper() {
        do_parse([:constant_of(Annot):]);
    }

    constexpr void parse(std::meta::info annot) {
        (this->*extract<void(all_argument_annotation_parser::*)()>(
            substitute(^^parse_helper, reflect_constant(annot))
        ))();
    }

};

} // namespace clap::details

#endif // !CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H
