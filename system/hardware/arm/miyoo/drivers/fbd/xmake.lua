target("fbd")
    set_type("application")
    add_deps("libbsp", "libupng", "libgraph", "libfbd")
    add_files("**.c")        
    install_dir("drivers/miyoo")
target_end()
