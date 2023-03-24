add_rules("mode.release", "mode.debug")
set_languages("c99", "cxx20")

target("sol_tool")
    set_kind("binary")
    add_files("*.cpp")
    add_includedirs("./")

    add_files("amf/**.cpp")
    add_includedirs("amf")

    add_files("binary/*.cpp")

    add_files("sol/*.cpp")

    add_linkdirs("everything")
    add_links("Everything64")

    if is_plat("windows") then
        add_defines("NOMINMAX")
        add_cxflags("/utf-8")
        add_cxxflags("/Zc:__cplusplus")

        if is_mode("release") then
            add_cxflags("/GL")
            add_ldflags("/LTCG")
        end
    end

    after_build(function (target)
        import("core.project.config")
        local targetfile = target:targetfile()
        os.cp(targetfile, path.join("$(projectdir)/", path.filename(targetfile)))
    end)

    set_rundir("$(projectdir)/")