#pragma once
#ifndef CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H
#define CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H 1

#include <meta>
#include <algorithm>
#include <optional>
#include <flat_map>
#include <flat_set>
#include <vector>

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
    consteval void do_parse(...) const noexcept {}
};

struct arg_name_parser {
    consteval void do_parse(annotations::long_arg_annot annot, std::meta::info member, const parsing_environment& env) {
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
        if (auto res = long_exists.insert({ annot.long_name, annot }); res.second) {
            long_args.push_back(annot);
        } else {
            auto it = res.first;
            if ((*it).second != annot) {
                throw std::meta::exception(
                    fmtext::format("conflicting long argument configuration for member '{}': '{}'",
                        identifier_of(member), it->second.long_name),
                    member);
            }
        }
    }

    consteval void do_parse(annotations::short_arg_annot annot, std::meta::info member, const parsing_environment& env) {
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
        if (auto res = short_exists.insert({ annot.short_name, annot }); res.second) {
            short_args.push_back(annot);
        } else {
            auto it = res.first;
            if ((*it).second != annot) {
                throw std::meta::exception(
                    fmtext::format("conflicting short argument configuration for member '{}': '{}'",
                        identifier_of(member), it->second.short_name),
                    member);
            }
        }
    }

    consteval void do_parse(annotations::named_arg_annot annot, std::meta::info member, const parsing_environment& env) {
        do_parse(annot.long_arg, member, env);
        do_parse(annot.short_arg, member, env);
    }

    std::vector<annotations::short_arg_annot> short_args;
    std::vector<annotations::long_arg_annot> long_args;
private:
    std::flat_map<char, annotations::short_arg_annot> short_exists;
    std::flat_map<std::string_view, annotations::long_arg_annot> long_exists;
};

struct positional_parser {
    consteval void do_parse(annotations::positional_annot annot, std::meta::info member, const parsing_environment&) {
        if (positional.has_value()) {
            throw std::meta::exception(
                fmtext::format("multiple positional annotations for member '{}'", identifier_of(member)),
                member);
        }
        positional = annot;
    }

    std::optional<annotations::positional_annot> positional;
};

// struct arg_count_parser {
//     consteval void do_parse(annotations::arg_count_annot annot, std::meta::info member, const parsing_environment&) {
//         // TODO: maybe consider use a unified arg handler model to do all of these
//     }

//     std::optional<annotations::arg_count_annot> arg_count;
// };

struct arg_action_parser {
    

    consteval void do_parse(annotations::arg_count_annot annot, std::meta::info member, const parsing_environment&) {
        // TODO
    }

    std::meta::info action;
};

struct env_default_parser {
    consteval void do_parse(annotations::env_default_annot annot, std::meta::info member, const parsing_environment& env) {
        if (annot.from_member_name()) {
            // generate env var name from member name
            std::string_view name = identifier_of(member);
            if (name.empty()) {
                throw std::meta::exception(
                    "environment variable name cannot be generated from empty member name",
                    member);
            }
            // trim to alphanumeric boundaries
            auto last_range = std::ranges::find_last_if(name, &ascii::is_alphanumeric<char>);
            if (last_range.empty()) {
                throw std::meta::exception(
                    fmtext::format("environment variable name cannot be generated from member name '{}' without alphanumeric characters", name),
                    member);
            }
            auto first = std::ranges::find_if(name, &ascii::is_alphanumeric<char>);
            name = std::string_view(first, last_range.begin() + 1);
            if (annot.env_var_style == style::unspecified) {
                annot.env_var_style = env.default_env_var_style.naming_style;
            }
            annot = annot(casecvt::convert(name, annot.env_var_style));
        }
        if (env_exists.insert(annot.env_var).second) {
            env_defaults.push_back(annot);
        }
    }

    std::vector<annotations::env_default_annot> env_defaults;
private:
    std::flat_set<std::string_view> env_exists;
};

struct help_parser {
    consteval void do_parse(annotations::help_annot annot, std::meta::info member, const parsing_environment&) {
        if (annot.help_text != annotations::help_annot::default_help) {
            if (help_text != annotations::help_annot::default_help) {
                throw std::meta::exception(
                    fmtext::format("multiple help annotations for member '{}'", identifier_of(member)),
                    member);
            }
            help_text = annot.help_text;
        }
    }

    const char* help_text = annotations::help_annot::default_help;
};

struct flags_parser {
    consteval void do_parse(annotations::propagated_annot, std::meta::info, const parsing_environment&) noexcept {
        propagated = true;
    }
    consteval void do_parse(annotations::shadow_parent_annot, std::meta::info, const parsing_environment&) noexcept {
        shadows_parent = true;
    }

    bool propagated = false;
    bool shadows_parent = false;
};

} // namespace clap::details::argument_annotation_parsers

template<typename Parser, std::meta::info Annot>
consteval void do_parse_helper(Parser& parser, std::meta::info member, const parsing_environment& env) {
    parser.do_parse([:constant_of(Annot):], member, env);
}

template<typename... Parsers>
struct combined_argument_annotation_parser : Parsers... {
    using Parsers::do_parse...;

    consteval void parse(std::meta::info member) {
        pre_parsing(member);
        for (auto annot : annotations_of(member)) {
            extract<void(*)(combined_argument_annotation_parser&, std::meta::info, const parsing_environment&)>(
                substitute(^^do_parse_helper, { ^^combined_argument_annotation_parser, reflect_constant(annot) })
            )(*this, member, env);
        }
        post_parsing(member);
    }

    parsing_environment env;

private:
    consteval void pre_parsing(std::meta::info member) {
        // assign default value based on type of member
        auto type = type_of(member);
        // TODO
    }

    consteval void post_parsing(std::meta::info member) {
        // argument cannot be both positional and named
        if (this->positional.has_value() && (!this->short_args.empty() || !this->long_args.empty())) {
            throw std::meta::exception(
                "argument cannot be both positional and named",
                member);
        }
    }
};

using argument_annotation_parser = [:[] consteval {
    return substitute(^^combined_argument_annotation_parser,
        members_of(^^argument_annotation_parsers, std::meta::access_context::current()));
}():];

} // namespace clap::details

#endif // !CCCLAP_PARSER_DETAILS_ARGUMENT_ANNOTATION_PARSER_H
