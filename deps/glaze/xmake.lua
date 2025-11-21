package("glaze")
    set_urls("https://github.com/stephenberry/glaze.git")

    add_deps("cmake")

    add_versions("origin/main", "ea748038ac4f1579e0603ed1b0405cb6d573a4ea")

    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        import("package.tools.cmake").install(package, configs)
        os.cp("include", package:installdir())
    end)
package_end()