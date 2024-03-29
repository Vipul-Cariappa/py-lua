workspace "py-lua"
   configurations { "Debug", "Release" }

project "pylua"
   kind "SharedLib"
   language "C"
   targetdir "bin"

   includedirs { "/usr/include/python3.10", "/usr/include/lua5.4" }
   files { "src/**.h", "src/**.c" }

   filter { "system:linux" }
      links { "python3.10", "lua" }

   filter "configurations:Debug"
      buildoptions { "-ftest-coverage", "-fprofile-arcs" }
      linkoptions { "-lgcov" }
      defines { "DEBUG" }
      symbols "On"
      optimize "Off"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
