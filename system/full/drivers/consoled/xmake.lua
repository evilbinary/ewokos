target("consoled")
    set_type("application")
    add_deps("libupng", "libdisplay", "libfb", "libfont", , "libgraph", "libsconf")
    add_files("**.c")        
    install_dir("drivers")
target_end()
