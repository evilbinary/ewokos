target("fbd")
    set_type("application")
    add_deps("libgraph", "libfbd","libarch_vpb")
    add_files("**.c")        
    install_dir("drivers/versatilepb")
target_end()
