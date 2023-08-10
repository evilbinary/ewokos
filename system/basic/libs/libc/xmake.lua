target("libewokc")
    set_kind("static")
    set_type("library")
    add_files("**.c")
    add_files("sys/src/syscall_arm.S")
    add_files("sys/src/arm32_aeabi_divmod_a32.S")
    add_files("sys/src/arm32_aeabi_ldivmod_a32.S")
    add_includedirs("include", "sys/include", {public = true})
target_end()
