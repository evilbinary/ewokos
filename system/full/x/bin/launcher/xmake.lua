target("launcher")
    set_type("application")
    add_files("**.cc")        
    add_deps("libgraph","libttf","libfont",, "libupng","libx++", "libx","libsconf", "libcxx")
    install_dir("bin/x")
target_end()
