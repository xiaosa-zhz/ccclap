#include <fmt/core.h>

#include <clap/util/cstring.hh>
#include <clap/util/ascii.hh>
#include <string>

using namespace clap;

int main() {
    fmt::println("Hello, world from fmt + C++26!");

    // --- basic_cstring_view demo ---

    // 1. construct from string literal
    cstring_view hello = "Hello, cstring_view!";
    fmt::println("{}", hello);

    // 2. construct from std::string
    std::string s = "from std::string";
    cstring_view csv{s};

    // 3. implicit conversion to string_view
    std::string_view sv = csv;
    fmt::println("size: {}, data: {}", sv.size(), sv);

    // 4. c_str() gives null-terminated pointer
    fmt::println("c_str: {}", csv.c_str());

    // 5. starts_with / ends_with
    fmt::println("starts_with(\"from\"): {}", csv.starts_with("from"));
    fmt::println("ends_with(\"string\"): {}", csv.ends_with("string"));

    // 6. contains
    fmt::println("contains(\"std\"): {}", csv.contains("std"));

    // 7. substr (single-arg) retains cstring_view
    cstring_view sub = hello.substr(7);
    fmt::println("substr(7): {}", sub);

    // 8. substr (two-arg) returns string_view
    std::string_view sub2 = hello.substr(0, 5);
    fmt::println("substr(0,5): {}", sub2);

    // 9. literal _csv
    using namespace clap::literals;
    cstring_view lit = "compile-time literal"_csv;
    fmt::println("_csv literal: {}", lit);

    // 10. comparison
    fmt::println("hello == \"Hello, cstring_view!\"_csv: {}",
                 hello == "Hello, cstring_view!"_csv);

    // 11. hash
    fmt::println("hash: {}", std::hash<cstring_view>{}(hello));

    // --- clap::ascii demo ---

    fmt::println("");

    // 1. is_digit / is_hex_digit / is_octal_digit / is_bit
    fmt::println("is_digit('5'):   {}", clap::ascii::is_digit('5'));
    fmt::println("is_digit('a'):   {}", clap::ascii::is_digit('a'));
    fmt::println("is_digit('z', 36): {}", clap::ascii::is_digit('z', 36));
    fmt::println("is_hex_digit('F'): {}", clap::ascii::is_hex_digit('F'));
    fmt::println("is_octal_digit('8'): {}", clap::ascii::is_octal_digit('8'));
    fmt::println("is_bit('1'): {}", clap::ascii::is_bit('1'));

    // 2. is_lower / is_upper / is_alphabetic / is_alphanumeric
    fmt::println("is_lower('g'):  {}", clap::ascii::is_lower('g'));
    fmt::println("is_upper('G'):  {}", clap::ascii::is_upper('G'));
    fmt::println("is_alphabetic('H'): {}", clap::ascii::is_alphabetic('H'));
    fmt::println("is_alphanumeric('9'): {}", clap::ascii::is_alphanumeric('9'));
    fmt::println("is_alphanumeric('_'): {}", clap::ascii::is_alphanumeric('_'));

    // 3. is_whitespace / is_horizontal_whitespace / is_control / is_printing
    fmt::println("is_whitespace('\\n'): {}", clap::ascii::is_whitespace('\n'));
    fmt::println("is_whitespace('\\t'): {}", clap::ascii::is_whitespace('\t'));
    fmt::println("is_horizontal_whitespace('\\t'): {}", clap::ascii::is_horizontal_whitespace('\t'));
    fmt::println("is_horizontal_whitespace('\\n'): {}", clap::ascii::is_horizontal_whitespace('\n'));
    fmt::println("is_control('\\x01'): {}", clap::ascii::is_control('\x01'));
    fmt::println("is_control('A'): {}", clap::ascii::is_control('A'));
    fmt::println("is_printing('!'): {}", clap::ascii::is_printing('!'));
    fmt::println("is_printing(' '): {}", clap::ascii::is_printing(' '));
    fmt::println("is_punctuation(','): {}", clap::ascii::is_punctuation(','));
    fmt::println("is_punctuation('A'): {}", clap::ascii::is_punctuation('A'));

    // 4. to_lower / to_upper
    fmt::println("to_lower('X'): {}", clap::ascii::to_lower('X'));
    fmt::println("to_upper('y'): {}", clap::ascii::to_upper('y'));
    fmt::println("to_lower('9'): {}", clap::ascii::to_lower('9'));

    // 5. case_insensitive_compare / case_insensitive_equals
    fmt::println("case_insensitive_equals('a','A'): {}", clap::ascii::case_insensitive_equals('a', 'A'));
    fmt::println("case_insensitive_equals('a','B'): {}", clap::ascii::case_insensitive_equals('a', 'B'));

    // 6. digit_value
    fmt::println("digit_value('7'): {}", clap::ascii::digit_value('7'));
    fmt::println("digit_value('A'): {}", clap::ascii::digit_value('A'));
    fmt::println("digit_value('f'): {}", clap::ascii::digit_value('f'));
    fmt::println("digit_value('G'): {}", clap::ascii::digit_value('G'));

    return 0;
}
