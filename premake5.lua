workspace "py-lua"
   configurations { "Debug", "Release" }

project "pylua"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"

   includedirs { "src/header", "/usr/include/python3.9", "/usr/include/x86_64-linux-gnu/python3.9", "/usr/include/lua5.4" }
   files { "src/**.h", "src/**.c" }

   filter { "system:linux" }
      links { "python3.9", "lua5.4" }

    filter { "system:windows" }
      links { "python", "lua" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
