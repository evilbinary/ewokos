target("libarch_miyoo")
    set_type("library")
    add_files("**.c")        
    add_deps("libewokc")
    add_includedirs("include", {public=true})
target_end()
