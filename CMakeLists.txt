cmake_minimum_required (VERSION 3.10)

project(pylua)

find_package(Python COMPONENTS Interpreter Development)
find_package(PkgConfig REQUIRED)

pkg_check_modules(LUA REQUIRED lua)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")

add_library(pylua SHARED src/convert.c src/py_lua.c src/lua_py.c)

message(STATUS "Python_INCLUDE_DIRS = ${Python_INCLUDE_DIRS}")
message(STATUS "Python_CFLAGS_OTHER = ${Python_CFLAGS_OTHER}")
message(STATUS "Python_LIBRARIES = ${Python_LIBRARIES}")
message(STATUS "LUA_LIBRARIES = ${LUA_LIBRARIES}")
message(STATUS "LUA_INCLUDE_DIRS = ${LUA_INCLUDE_DIRS}")
message(STATUS "LUA_CFLAGS_OTHER = ${LUA_CFLAGS_OTHER}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT WIN32)
    message(STATUS "Building in Debug, Therefore compiling with Code Coverage")
    target_compile_options(pylua PRIVATE -coverage)
    target_link_options(pylua PRIVATE -coverage)
endif()

target_include_directories(pylua PRIVATE "src/")

target_include_directories(pylua PRIVATE ${Python_INCLUDE_DIRS})
target_link_libraries(pylua ${Python_LIBRARIES})
target_compile_options(pylua PRIVATE ${Python_CFLAGS_OTHER})

target_link_libraries(pylua ${LUA_LIBRARIES})
target_include_directories(pylua PRIVATE ${LUA_INCLUDE_DIRS})
target_compile_options(pylua PRIVATE ${LUA_CFLAGS_OTHER})
