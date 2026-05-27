#include "clap/annotations.hh"
#include "clap/util/casecvt.hh"
#include <clap/util/cstring.hh>
#include <clap/util/ascii.hh>
#include <clap/util/fmtext.hh>
#include <clap/parser/token.hh>
#include <clap/parser/parser.hh>
#include <string>
#include <string_view>
#include <vector>
#include <ranges>
#include <bit>
#include <fmt/ranges.h>
#include <nowide/args.hpp>

using namespace clap;

inline constexpr const char* test = std::define_static_string(fmtext::format("{}", "compile time"));

inline constexpr const char* test_format_to = std::define_static_string([] {
    std::string s;
    fmtext::format_to(std::back_inserter(s), "Hello, {}!", test);
    return s;
}());

consteval void test_exec_coding_utf8() {
    constexpr char    test0[] = "\u00E9\u00A9\u00ED\00FD";
    constexpr char8_t test1[] = u8"\u00E9\u00A9\u00ED\00FD";
    for (auto [c, c8] : std::views::zip(test0, test1)) {
        if (static_cast<char8_t>(c) != c8) {
            throw std::meta::exception("Must use UTF-8 encoding for source files", {});
        }
    }
}

struct cli_arg {
    std::string_view original;
    std::string_view value;
    friend constexpr std::string_view format_as(const cli_arg& arg) noexcept {
        return arg.value;
    }
};

void parse(int argc, char** argv) {
    test_exec_coding_utf8();
    char** old_argv = argv;
    nowide::args _(argc, argv);
    char** new_argv = argv;
    auto args = std::views::zip(std::span(old_argv, argc), std::span(new_argv, argc))
        | std::views::transform([](const auto& pair) {
            const auto& [old_arg, new_arg] = pair;
            return cli_arg{std::string_view(old_arg), std::string_view(new_arg)};
        })
        | std::ranges::to<std::vector>();
    fmtext::println("{}", args);
}

constexpr const char* camelCase = "fooBar";

constexpr const char* PascalCase = [] consteval {
    return std::define_static_string(clap::casecvt::to_pascal(camelCase));
}();

constexpr const char* snake_case = [] consteval {
    return std::define_static_string(clap::casecvt::to_snake(camelCase));
}();

constexpr const char* SCREAMING_SNAKE_CASE = [] consteval {
    return std::define_static_string(clap::casecvt::to_screaming_snake(camelCase));
}();

constexpr const char* kebab_case = [] consteval {
    return std::define_static_string(clap::casecvt::to_kebab(camelCase));
}();

constexpr void lut_test() {
    using action_type = void(*)();
    static constexpr auto raw = [] consteval {
        std::vector<clap::details::lookup_table_entry<action_type>> r = {
            {std::define_static_string("foo"), +[] { fmtext::println("foo"); }},
            {std::define_static_string("bar"), +[] { fmtext::println("bar"); }},
            {std::define_static_string("baz"), +[] { fmtext::println("baz"); }},
        };
        return std::define_static_array(r);
    }();
    static constexpr auto lut = clap::details::make_lookup_table<action_type, raw.data(), raw.size()>();
    lut.at("foo")();
    lut.at("bar")();
    lut.at("baz")();
}

struct test_command {
    [[=clap::arg, =clap::arg('C'), =clap::long_arg(clap::style::snake)]]
    bool copy_cat;
};

constexpr auto short_names = std::define_static_array([] consteval {
    clap::details::argument_annotation_parser parser;
    parser.parse(^^test_command::copy_cat);
    return std::move(parser.short_args);
}());

constexpr auto long_names = std::define_static_array([] consteval {
    clap::details::argument_annotation_parser parser;
    parser.env.default_arg_style.naming_style = clap::style::kebab;
    parser.parse(^^test_command::copy_cat);
    return std::move(parser.long_args);
}());

int main(int argc, char** argv) {
    parse(argc, argv);
    lut_test();
    fmtext::println("{}", short_names | std::views::transform(&clap::annotations::short_arg_annot::short_name));
    fmtext::println("{}", long_names | std::views::transform(&clap::annotations::long_arg_annot::long_name));
    fmtext::println(test_format_to);
    fmtext::println("{}", display_string_of(^^clap::details::argument_annotation_parser));
    fmtext::println("Hello, world from fmt + C++26!");

    // --- basic_cstring_view demo ---

    // 1. construct from string literal
    cstring_view hello = "Hello, cstring_view!";
    fmtext::println("{}", hello);

    // 2. construct from std::string
    std::string s = "from std::string";
    cstring_view csv{s};

    // 3. implicit conversion to string_view
    std::string_view sv = csv;
    fmtext::println("size: {}, data: {}", sv.size(), sv);

    // 4. c_str() gives null-terminated pointer
    fmtext::println("c_str: {}", csv.c_str());

    // 5. starts_with / ends_with
    fmtext::println("starts_with(\"from\"): {}", csv.starts_with("from"));
    fmtext::println("ends_with(\"string\"): {}", csv.ends_with("string"));

    // 6. contains
    fmtext::println("contains(\"std\"): {}", csv.contains("std"));

    // 7. substr (single-arg) retains cstring_view
    cstring_view sub = hello.substr(7);
    fmtext::println("substr(7): {}", sub);

    // 8. substr (two-arg) returns string_view
    std::string_view sub2 = hello.substr(0, 5);
    fmtext::println("substr(0,5): {}", sub2);

    // 9. literal _csv
    using namespace clap::literals;
    static constexpr cstring_view lit = "compile-time literal: {}"_csv;
    fmtext::println(lit, "_csv literal"_csv);

    // 10. comparison
    fmtext::println("hello == \"Hello, cstring_view!\"_csv: {}",
                 hello == "Hello, cstring_view!"_csv);

    // 11. hash
    fmtext::println("hash: {}", std::hash<cstring_view>{}(hello));

    // --- clap::ascii demo ---

    fmtext::println("");

    // 1. is_digit / is_hex_digit / is_octal_digit / is_bit
    fmtext::println("is_digit('5'):   {}", clap::ascii::is_digit('5'));
    fmtext::println("is_digit('a'):   {}", clap::ascii::is_digit('a'));
    fmtext::println("is_digit('z', 36): {}", clap::ascii::is_digit('z', 36));
    fmtext::println("is_hex_digit('F'): {}", clap::ascii::is_hex_digit('F'));
    fmtext::println("is_octal_digit('8'): {}", clap::ascii::is_octal_digit('8'));
    fmtext::println("is_bit('1'): {}", clap::ascii::is_bit('1'));

    // 2. is_lower / is_upper / is_alphabetic / is_alphanumeric
    fmtext::println("is_lower('g'):  {}", clap::ascii::is_lower('g'));
    fmtext::println("is_upper('G'):  {}", clap::ascii::is_upper('G'));
    fmtext::println("is_alphabetic('H'): {}", clap::ascii::is_alphabetic('H'));
    fmtext::println("is_alphanumeric('9'): {}", clap::ascii::is_alphanumeric('9'));
    fmtext::println("is_alphanumeric('_'): {}", clap::ascii::is_alphanumeric('_'));

    // 3. is_whitespace / is_horizontal_whitespace / is_control / is_printing
    fmtext::println("is_whitespace('\\n'): {}", clap::ascii::is_whitespace('\n'));
    fmtext::println("is_whitespace('\\t'): {}", clap::ascii::is_whitespace('\t'));
    fmtext::println("is_horizontal_whitespace('\\t'): {}", clap::ascii::is_horizontal_whitespace('\t'));
    fmtext::println("is_horizontal_whitespace('\\n'): {}", clap::ascii::is_horizontal_whitespace('\n'));
    fmtext::println("is_control('\\x01'): {}", clap::ascii::is_control('\x01'));
    fmtext::println("is_control('A'): {}", clap::ascii::is_control('A'));
    fmtext::println("is_printing('!'): {}", clap::ascii::is_printing('!'));
    fmtext::println("is_printing(' '): {}", clap::ascii::is_printing(' '));
    fmtext::println("is_punctuation(','): {}", clap::ascii::is_punctuation(','));
    fmtext::println("is_punctuation('A'): {}", clap::ascii::is_punctuation('A'));

    // 4. to_lower / to_upper
    fmtext::println("to_lower('X'): {}", clap::ascii::to_lower('X'));
    fmtext::println("to_upper('y'): {}", clap::ascii::to_upper('y'));
    fmtext::println("to_lower('9'): {}", clap::ascii::to_lower('9'));

    // 5. case_insensitive_compare / case_insensitive_equals
    fmtext::println("case_insensitive_equals('a','A'): {}", clap::ascii::case_insensitive_equals('a', 'A'));
    fmtext::println("case_insensitive_equals('a','B'): {}", clap::ascii::case_insensitive_equals('a', 'B'));

    // 6. digit_value
    fmtext::println("digit_value('7'): {}", clap::ascii::digit_value('7'));
    fmtext::println("digit_value('A'): {}", clap::ascii::digit_value('A'));
    fmtext::println("digit_value('f'): {}", clap::ascii::digit_value('f'));
    fmtext::println("digit_value('G'): {}", clap::ascii::digit_value('G'));

    fmtext::plural_println("There is {} file, {}", "There are {} files, {}", fmtext::plural(2uz), 2uz);

    return 0;
}
