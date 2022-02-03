cmake_minimum_required (VERSION 3.8)

find_package(PkgConfig REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")

pkg_check_modules(LUA REQUIRED lua)
pkg_check_modules(PYTHON REQUIRED python3)

add_library(pylua SHARED src/convert.c src/py_lua.c src/lua_py.c)

target_include_directories(pylua PRIVATE "src/")

target_include_directories(pylua PRIVATE ${PYTHON_INCLUDE_DIRS})
target_link_libraries(pylua ${PYTHON_LIBRARIES})
target_compile_options(pylua PRIVATE ${PYTHON_CFLAGS_OTHER})

target_link_libraries(pylua ${LUA_LIBRARIES})
target_include_directories(pylua PRIVATE ${LUA_INCLUDE_DIRS})
target_compile_options(pylua PRIVATE ${LUA_CFLAGS_OTHER})
