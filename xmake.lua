add_rules("mode.debug", "mode.release")

add_requires("fmt")

target("ccclap")
    set_kind("binary")
    set_languages("c++26")
    add_cxflags("-freflection", {force = true})
    add_cxflags("-fcontracts", {force = true})
    add_includedirs("include")
    add_files("src/*.cc")
    add_packages("fmt")
