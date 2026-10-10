#include "clap/annotations.hh"
#include "clap/util/casecvt.hh"
#include <clap/util/cstring.hh>
#include <clap/util/ascii.hh>
#include <clap/util/fmtext.hh>
#include <clap/util/enum.hh>
#include <clap/parser/token.hh>
#include <clap/parser/parser.hh>
#include <clap/parser/details/arg_val_parser.hh>
#include <string>
#include <string_view>
#include <vector>
#include <ranges>
#include <bit>
#include <nowide/args.hpp>

using namespace clap;

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
};

template<>
struct std::formatter<cli_arg, char>
    : std::formatter<std::string_view, char>
{
    template <typename FormatContext>
    constexpr auto format(const cli_arg& arg, FormatContext& ctx) const {
        using base = std::formatter<std::string_view, char>;
        return base::format(arg.value, ctx);
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
    i18n::println("{}", args);
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
    static constexpr auto lut = [] consteval {
        std::vector<std::pair<cstring_view, action_type>> raw = {
            {std::define_static_string("foo"), +[] { i18n::println("foo"); }},
            {std::define_static_string("bar"), +[] { i18n::println("bar"); }},
            {std::define_static_string("baz"), +[] { i18n::println("baz"); }},
        };
        return clap::details::make_lookup_table<action_type>(raw);
    }();
    lut.at("foo")();
    lut.at("bar")();
    lut.at("baz")();
}

struct test_command {
    [[=clap::arg, =clap::arg('C'), =clap::long_arg(clap::style::snake)]]
    [[=clap::help("Whether to copy files instead of creating hard links")]]
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

constexpr auto help_text = [] consteval {
    clap::details::argument_annotation_parser parser;
    parser.parse(^^test_command::copy_cat);
    return parser.help_text;
}();

static_assert(clap::details::is_appendable_container(^^std::vector<int>));

int main(int argc, char** argv) {
    parse(argc, argv);
    lut_test();
    i18n::println("{}", short_names | std::views::transform(&clap::annotations::short_arg_annot::short_name));
    i18n::println("{}", long_names | std::views::transform(&clap::annotations::long_arg_annot::long_name));
    i18n::println("{}", help_text);
    i18n::println("{}", display_string_of(^^clap::details::argument_annotation_parser));
    i18n::println("Hello, world from fmt + C++26!");

    // --- basic_cstring_view demo ---

    // 1. construct from string literal
    cstring_view hello = "Hello, cstring_view!";
    i18n::println("{}", hello);

    // 2. construct from std::string
    std::string s = "from std::string";
    cstring_view csv{s};

    // 3. implicit conversion to string_view
    std::string_view sv = csv;
    i18n::println("size: {}, data: {}", sv.size(), sv);

    // 4. c_str() gives null-terminated pointer
    i18n::println("c_str: {}", csv.c_str());

    // 5. starts_with / ends_with
    i18n::println("starts_with(\"from\"): {}", csv.starts_with("from"));
    i18n::println("ends_with(\"string\"): {}", csv.ends_with("string"));

    // 6. contains
    i18n::println("contains(\"std\"): {}", csv.contains("std"));

    // 7. substr (single-arg) retains cstring_view
    cstring_view sub = hello.substr(7);
    i18n::println("substr(7): {}", sub);

    // 8. substr (two-arg) returns string_view
    std::string_view sub2 = hello.substr(0, 5);
    i18n::println("substr(0,5): {}", sub2);

    // 9. literal _csv
    using namespace clap::literals;
    static constexpr cstring_view lit = "compile-time literal: {}"_csv;
    i18n::println(lit, "_csv literal"_csv);

    // 10. comparison
    i18n::println("hello == \"Hello, cstring_view!\"_csv: {}",
                 hello == "Hello, cstring_view!"_csv);

    // 11. hash
    i18n::println("hash: {}", std::hash<cstring_view>{}(hello));

    // --- clap::ascii demo ---

    i18n::println("");

    // 1. is_digit / is_hex_digit / is_octal_digit / is_bit
    i18n::println("is_digit('5'):   {}", clap::ascii::is_digit('5'));
    i18n::println("is_digit('a'):   {}", clap::ascii::is_digit('a'));
    i18n::println("is_digit('z', 36): {}", clap::ascii::is_digit('z', 36));
    i18n::println("is_hex_digit('F'): {}", clap::ascii::is_hex_digit('F'));
    i18n::println("is_octal_digit('8'): {}", clap::ascii::is_octal_digit('8'));
    i18n::println("is_bit('1'): {}", clap::ascii::is_bit('1'));

    // 2. is_lower / is_upper / is_alphabetic / is_alphanumeric
    i18n::println("is_lower('g'):  {}", clap::ascii::is_lower('g'));
    i18n::println("is_upper('G'):  {}", clap::ascii::is_upper('G'));
    i18n::println("is_alphabetic('H'): {}", clap::ascii::is_alphabetic('H'));
    i18n::println("is_alphanumeric('9'): {}", clap::ascii::is_alphanumeric('9'));
    i18n::println("is_alphanumeric('_'): {}", clap::ascii::is_alphanumeric('_'));

    // 3. is_whitespace / is_horizontal_whitespace / is_control / is_printing
    i18n::println("is_whitespace('\\n'): {}", clap::ascii::is_whitespace('\n'));
    i18n::println("is_whitespace('\\t'): {}", clap::ascii::is_whitespace('\t'));
    i18n::println("is_horizontal_whitespace('\\t'): {}", clap::ascii::is_horizontal_whitespace('\t'));
    i18n::println("is_horizontal_whitespace('\\n'): {}", clap::ascii::is_horizontal_whitespace('\n'));
    i18n::println("is_control('\\x01'): {}", clap::ascii::is_control('\x01'));
    i18n::println("is_control('A'): {}", clap::ascii::is_control('A'));
    i18n::println("is_printing('!'): {}", clap::ascii::is_printing('!'));
    i18n::println("is_printing(' '): {}", clap::ascii::is_printing(' '));
    i18n::println("is_punctuation(','): {}", clap::ascii::is_punctuation(','));
    i18n::println("is_punctuation('A'): {}", clap::ascii::is_punctuation('A'));

    // 4. to_lower / to_upper
    i18n::println("to_lower('X'): {}", clap::ascii::to_lower('X'));
    i18n::println("to_upper('y'): {}", clap::ascii::to_upper('y'));
    i18n::println("to_lower('9'): {}", clap::ascii::to_lower('9'));

    // 5. case_insensitive_compare / case_insensitive_equals
    i18n::println("case_insensitive_equals('a','A'): {}", clap::ascii::case_insensitive_equals('a', 'A'));
    i18n::println("case_insensitive_equals('a','B'): {}", clap::ascii::case_insensitive_equals('a', 'B'));

    // 6. digit_value
    i18n::println("digit_value('7'): {}", clap::ascii::digit_value('7'));
    i18n::println("digit_value('A'): {}", clap::ascii::digit_value('A'));
    i18n::println("digit_value('f'): {}", clap::ascii::digit_value('f'));
    i18n::println("digit_value('G'): {}", clap::ascii::digit_value('G'));

    i18n::plural_println("There is {} file, {}", "There are {} files, {}", i18n::plural(2uz), 2uz);

    // --- clap::enum_to_string / string_to_enum ---
    i18n::println("");
    static_assert(clap::enum_type<clap::style>);
    static_assert(!clap::enum_type<int>);

    // enum_to_string: 每个枚举值应与其标识符相符
    i18n::println("enum_to_string(style::unspecified):     {}", clap::enum_to_string(style::unspecified).value());
    i18n::println("enum_to_string(style::verbatim):        {}", clap::enum_to_string(style::verbatim).value());
    i18n::println("enum_to_string(style::kebab):           {}", clap::enum_to_string(style::kebab).value());
    i18n::println("enum_to_string(style::snake):           {}", clap::enum_to_string(style::snake).value());
    i18n::println("enum_to_string(style::screaming_snake): {}", clap::enum_to_string(style::screaming_snake).value());
    i18n::println("enum_to_string(style::camel):           {}", clap::enum_to_string(style::camel).value());
    i18n::println("enum_to_string(style::pascal):          {}", clap::enum_to_string(style::pascal).value());

    // string_to_enum: 正向查找与无效输入
    i18n::println("string_to_enum<style>(\"kebab\") == style::kebab:           {}", clap::string_to_enum<style>("kebab") == style::kebab);
    i18n::println("string_to_enum<style>(\"screaming_snake\") == screaming_snake: {}", clap::string_to_enum<style>("screaming_snake") == style::screaming_snake);
    i18n::println("string_to_enum<style>(\"INVALID\") has_value:               {}", clap::string_to_enum<style>("INVALID").has_value());

    return 0;
}
