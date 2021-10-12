workspace "py-lua"
   configurations { "Debug", "Release" }

project "pylua"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"

   includedirs { "src/header", "/path/to/python/header", "/path/to/lua/header" }
   files { "src/**.h", "src/**.c" }

   filter { "system:linux" }
      links { "python3.8", "lua5.4" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
