#include <fmt/core.h>

#include <clap/util/cstring.hh>
#include <string>
#include <iostream>

using namespace clap;

int main() {
    fmt::println("Hello, world from fmt + C++26!");

    // --- basic_cstring_view demo ---

    // 1. construct from string literal
    cstring_view hello = "Hello, cstring_view!";
    std::cout << hello << '\n';

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

    return 0;
}
