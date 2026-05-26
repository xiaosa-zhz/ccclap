#pragma once
#ifndef CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H
#define CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H 1

#include <meta>
#include <concepts>

namespace clap::details {

template<typename AnnotType>
consteval bool is_annotation_of_type(std::meta::info annot) {
    return remove_const(type_of(annot)) == ^^AnnotType;
}

template<std::meta::info Annot>
struct argument_annotation_parser {
    static_assert(false, "Unsupported annotation type");
};

template<std::meta::info Annot>
    requires is_annotation_of_type<annotations::short_arg_annot>(Annot)
struct argument_annotation_parser<Annot> {

};

} // namespace clap::details

#endif // !CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H
