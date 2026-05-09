#include <clap/util/cstring.hh>
#include <clap/util/ascii.hh>
#include <clap/util/i18n.hh>
#include <string>

using namespace clap;
using namespace clap::i18n;

int main() {
    println("Hello, world from fmt + C++26!");

    // --- basic_cstring_view demo ---

    // 1. construct from string literal
    cstring_view hello = "Hello, cstring_view!";
    println("{}", hello);

    // 2. construct from std::string
    std::string s = "from std::string";
    cstring_view csv{s};

    // 3. implicit conversion to string_view
    std::string_view sv = csv;
    println("size: {}, data: {}", sv.size(), sv);

    // 4. c_str() gives null-terminated pointer
    println("c_str: {}", csv.c_str());

    // 5. starts_with / ends_with
    println("starts_with(\"from\"): {}", csv.starts_with("from"));
    println("ends_with(\"string\"): {}", csv.ends_with("string"));

    // 6. contains
    println("contains(\"std\"): {}", csv.contains("std"));

    // 7. substr (single-arg) retains cstring_view
    cstring_view sub = hello.substr(7);
    println("substr(7): {}", sub);

    // 8. substr (two-arg) returns string_view
    std::string_view sub2 = hello.substr(0, 5);
    println("substr(0,5): {}", sub2);

    // 9. literal _csv
    using namespace clap::literals;
    constexpr static cstring_view lit = "compile-time literal: {}"_csv;
    println(lit, "_csv literal"_csv);

    // 10. comparison
    println("hello == \"Hello, cstring_view!\"_csv: {}",
                 hello == "Hello, cstring_view!"_csv);

    // 11. hash
    println("hash: {}", std::hash<cstring_view>{}(hello));

    // --- clap::ascii demo ---

    println("");

    // 1. is_digit / is_hex_digit / is_octal_digit / is_bit
    println("is_digit('5'):   {}", clap::ascii::is_digit('5'));
    println("is_digit('a'):   {}", clap::ascii::is_digit('a'));
    println("is_digit('z', 36): {}", clap::ascii::is_digit('z', 36));
    println("is_hex_digit('F'): {}", clap::ascii::is_hex_digit('F'));
    println("is_octal_digit('8'): {}", clap::ascii::is_octal_digit('8'));
    println("is_bit('1'): {}", clap::ascii::is_bit('1'));

    // 2. is_lower / is_upper / is_alphabetic / is_alphanumeric
    println("is_lower('g'):  {}", clap::ascii::is_lower('g'));
    println("is_upper('G'):  {}", clap::ascii::is_upper('G'));
    println("is_alphabetic('H'): {}", clap::ascii::is_alphabetic('H'));
    println("is_alphanumeric('9'): {}", clap::ascii::is_alphanumeric('9'));
    println("is_alphanumeric('_'): {}", clap::ascii::is_alphanumeric('_'));

    // 3. is_whitespace / is_horizontal_whitespace / is_control / is_printing
    println("is_whitespace('\\n'): {}", clap::ascii::is_whitespace('\n'));
    println("is_whitespace('\\t'): {}", clap::ascii::is_whitespace('\t'));
    println("is_horizontal_whitespace('\\t'): {}", clap::ascii::is_horizontal_whitespace('\t'));
    println("is_horizontal_whitespace('\\n'): {}", clap::ascii::is_horizontal_whitespace('\n'));
    println("is_control('\\x01'): {}", clap::ascii::is_control('\x01'));
    println("is_control('A'): {}", clap::ascii::is_control('A'));
    println("is_printing('!'): {}", clap::ascii::is_printing('!'));
    println("is_printing(' '): {}", clap::ascii::is_printing(' '));
    println("is_punctuation(','): {}", clap::ascii::is_punctuation(','));
    println("is_punctuation('A'): {}", clap::ascii::is_punctuation('A'));

    // 4. to_lower / to_upper
    println("to_lower('X'): {}", clap::ascii::to_lower('X'));
    println("to_upper('y'): {}", clap::ascii::to_upper('y'));
    println("to_lower('9'): {}", clap::ascii::to_lower('9'));

    // 5. case_insensitive_compare / case_insensitive_equals
    println("case_insensitive_equals('a','A'): {}", clap::ascii::case_insensitive_equals('a', 'A'));
    println("case_insensitive_equals('a','B'): {}", clap::ascii::case_insensitive_equals('a', 'B'));

    // 6. digit_value
    println("digit_value('7'): {}", clap::ascii::digit_value('7'));
    println("digit_value('A'): {}", clap::ascii::digit_value('A'));
    println("digit_value('f'): {}", clap::ascii::digit_value('f'));
    println("digit_value('G'): {}", clap::ascii::digit_value('G'));

    plural_println("There is {} file, {}", "There are {} files, {}", plural(2uz), 2uz);

    return 0;
}
