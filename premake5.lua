workspace "py-lua"
   configurations { "Debug", "Release" }

project "pylua"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"

   includedirs { "/path/to/python/header", "/path/to/lua/header" }
   libdirs { "/path/to/python/shared_libraries", "/path/to/lua/shared_libraries" }
   files { "src/**.h", "src/**.c" }

   filter "configurations:Debug"
      links { "python3.8_d", "lua5.4" }
      defines { "DEBUG", "Py_DEBUG" }
      symbols "On"

   filter "configurations:Release"
      links { "python3.8", "lua5.4" }

      defines { "NDEBUG" }
      optimize "On"
