target("libx++")
    set_type("library")
    add_files("**.cc")        
    add_deps("libx", "libfont")
    add_includedirs("include", {public = true})
target_end()
