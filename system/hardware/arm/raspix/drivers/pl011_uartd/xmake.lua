target("pl011_uartd")
    set_type("application")
    add_deps("libbsp")
    add_files("**.c")        
    install_dir("drivers/raspix")
target_end()
