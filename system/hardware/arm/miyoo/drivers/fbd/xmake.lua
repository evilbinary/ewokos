target("fbd")
    set_type("application")
    add_deps("libbsp", "libgraph", "libfbd")
    add_files("**.c")        
    install_dir("drivers/miyoo")
target_end()