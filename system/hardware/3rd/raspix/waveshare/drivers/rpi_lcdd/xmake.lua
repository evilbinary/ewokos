target("rpi_lcdd")
    set_type("application")
    add_deps("libfbd", "libili9486","libbsp")
    add_files("**.c")        
    install_dir("drivers/raspix")
target_end()
