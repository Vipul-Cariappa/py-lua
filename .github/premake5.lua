workspace "py-lua"
   configurations { "Debug", "Release" }

project "pylua"
   kind "SharedLib"
   language "C"
   targetdir "bin"

   includedirs { "src/header", "/usr/include/python3.8", "/usr/include/lua5.4" }
   files { "src/**.h", "src/**.c" }

   filter { "system:linux" }
      links { "python3.8", "lua" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
