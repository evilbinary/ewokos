target("gpio_joystickd")
    set_type("application")
    add_deps("libbsp")
    add_files("**.c")        
    install_dir("drivers/miyoo")
target_end()
