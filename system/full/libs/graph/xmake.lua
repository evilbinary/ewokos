target("libgraph")
    set_type("library")
	add_cflags("-mfpu=neon-vfpv4", {force=true})
    add_files("**.c")        
    add_deps("libsoftfloat", "libewokc")
    add_includedirs("include",  {public = true})
target_end()
