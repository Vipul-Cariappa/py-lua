---@diagnostic disable: undefined-global
workspace "py-lua"
   configurations { "Debug", "Release" }

project "pylua"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"

   files { "src/**.h", "src/**.c" }

   filter "configurations:Debug"
      includedirs { "/usr/local/include/python3.10", "/usr/include/lua5.4" }
      links { "python3.10", "lua5.4" }
      defines { "DEBUG", "Py_DEBUG" }
      symbols "On"

   filter "configurations:Release"
      includedirs { "/usr/include/python3.10", "/usr/include/lua5.4" }
      links { "python3.10", "lua5.4" }
      defines { "NDEBUG" }
      optimize "On"
