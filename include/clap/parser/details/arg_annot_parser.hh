#pragma once
#ifndef CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H
#define CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H 1

#include <cstddef>
#include <meta>
#include <concepts>
#include <algorithm>
#include <ranges>

#include <clap/annotations.hh>
#include <clap/util/fmtext.hh>
#include <clap/util/casecvt.hh>

namespace clap::details {

struct parsing_environment {
    annotations::arg_naming_style_annot default_arg_style = {};
    annotations::env_var_naming_style_annot default_env_var_style = {};
};

namespace argument_annotation_parsers {

// Fallback parser that does nothing, used for annotations without specific parser.
struct noop_parser {
    consteval void do_parse(std::meta::info member, std::meta::info annot, const parsing_environment& env) const noexcept {}
    consteval void do_build() const noexcept {}
};

template<typename Annot>
inline constexpr std::meta::info find_argument_parser = ^^noop_parser;

struct arg_name_parser {
    consteval void do_parse_long(std::meta::info member, annotations::long_arg_annot annot, const parsing_environment& env) {
        if (annot.from_member_name()) {
            // generete long name from member name
            std::string_view name = identifier_of(member);
            if (name.empty()) {
                throw std::meta::exception(
                    "long argument name cannot be generated from empty member name",
                    member);
            }
            // trim original name
            auto last_range = std::ranges::find_last_if(name, &ascii::is_alphanumeric<char>);
            if (last_range.empty()) {
                throw std::meta::exception(
                    fmtext::format("long argument name cannot be generated from member name '{}' without alphanumeric characters", name),
                    member);
            }
            auto first = std::ranges::find_if(name, &ascii::is_alphanumeric<char>);
            name = std::string_view(first, last_range.begin() + 1);
            if (annot.long_name_style == style::unspecified) {
                annot.long_name_style = env.default_arg_style.naming_style;
            }
            annot = annot(casecvt::convert(name, annot.long_name_style), annot.hidden);
        }
        if (auto it = long_exists.find(annot.long_name); it != long_exists.end()) {
            if ((*it).second != annot) {
                throw std::meta::exception(
                    fmtext::format("conflicting long argument configuration for member '{}': '{}'",
                        identifier_of(member), it->second.long_name),
                    member);
            }
        } else {
            long_exists.insert({ annot.long_name, annot });
            long_args.push_back(annot);
        }
    }

    consteval void do_parse_short(std::meta::info member, annotations::short_arg_annot annot, const parsing_environment& env) {
        if (annot.from_member_name()) {
            // generete short name from member name
            std::string_view name = identifier_of(member);
            if (name.empty()) {
                throw std::meta::exception(
                    "short argument name cannot be generated from empty member name",
                    member);
            }
            // look for the first alphanumeric character in the member name to use as short name
            auto it = std::ranges::find_if(name, &ascii::is_alphanumeric<char>);
            if (it == name.end()) {
                throw std::meta::exception(
                    fmtext::format("short argument name cannot be generated from member name '{}' without alphanumeric characters", name),
                    member);
            }
            annot = annot(*it, annot.hidden);
        }
        if (auto it = short_exists.find(annot.short_name); it != short_exists.end()) {
            if ((*it).second != annot) {
                throw std::meta::exception(
                    fmtext::format("conflicting short argument configuration for member '{}': '{}'",
                        identifier_of(member), it->second.short_name),
                    member);
            }
        } else {
            short_exists.insert({ annot.short_name, annot });
            short_args.push_back(annot);
        }
    }

    consteval void do_parse_named(std::meta::info member, annotations::named_arg_annot annot, const parsing_environment& env) {
        do_parse_long(member, annot.long_arg, env);
        do_parse_short(member, annot.short_arg, env);
    }

    consteval void do_parse(std::meta::info member, std::meta::info annot, const parsing_environment& env) {
        auto annot_type = remove_const(type_of(annot));
        if (annot_type == ^^annotations::long_arg_annot) {
            do_parse_long(member, extract<annotations::long_arg_annot>(annot), env);
        } else if (annot_type == ^^annotations::short_arg_annot) {
            do_parse_short(member, extract<annotations::short_arg_annot>(annot), env);
        } else if (annot_type == ^^annotations::named_arg_annot) {
            do_parse_named(member, extract<annotations::named_arg_annot>(annot), env);
        } else {
            // should not reach here since this parser is only selected for argument annotations
            throw std::meta::exception(
                "invalid annotation type for arg_name_parser",
                {});
        }
    }

    consteval void do_build() const noexcept {}

    std::vector<annotations::short_arg_annot> short_args;
    std::vector<annotations::long_arg_annot> long_args;
private:
    std::flat_map<char, annotations::short_arg_annot> short_exists;
    std::flat_map<std::string_view, annotations::long_arg_annot> long_exists;
};

template<typename Annot>
    requires std::same_as<Annot, annotations::long_arg_annot>
        || std::same_as<Annot, annotations::short_arg_annot>
        || std::same_as<Annot, annotations::named_arg_annot>
inline constexpr std::meta::info find_argument_parser<Annot> = ^^arg_name_parser;

struct help_parser {
    consteval void do_parse(std::meta::info member, std::meta::info annot, const parsing_environment& env) {}
    consteval void do_build() const noexcept {}
};

template<>
inline constexpr std::meta::info find_argument_parser<annotations::help_annot> = ^^help_parser;

} // namespace clap::details::argument_annotation_parsers

template<typename... Parsers>
struct combined_argument_annotation_parser : Parsers... {
    consteval void parse(std::meta::info member) {
        for (auto annot : annotations_of(member)) {
            auto target_parser = extract<std::meta::info>(
                substitute(^^argument_annotation_parsers::find_argument_parser, { remove_const(type_of(annot)) }));
            template for (constexpr std::meta::info parser : { ^^Parsers... }) { 
                if (parser == target_parser) {
                    this->[:parser:]::do_parse(member, annot, env);
                }
            }
        }
    }

    parsing_environment env;
};

using argument_annotation_parser = [:[] consteval {
    auto parser_types = members_of(^^annotations::argument_annotations, std::meta::access_context::current())
        | std::views::transform([](std::meta::info annot_type) {
            return extract<std::meta::info>(
                substitute(^^argument_annotation_parsers::find_argument_parser, { annot_type })
            );
        })
        | std::ranges::to<std::vector>();
    std::ranges::sort(parser_types, [](auto a, auto b) { return type_order(a, b) < 0; });
    auto [first, last] = std::ranges::unique(parser_types);
    parser_types.erase(first, last);
    return substitute(^^combined_argument_annotation_parser, parser_types);
}():];

} // namespace clap::details

#endif // !CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H
