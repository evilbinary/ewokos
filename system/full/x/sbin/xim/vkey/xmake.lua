target("xim_vkey")
    set_type("application")
    add_files("*.cc")        
    add_deps("libfont","libx++", "libcxx")
    install_dir("sbin/x")
target_end()
