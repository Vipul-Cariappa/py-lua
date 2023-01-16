from distutils.core import setup, Extension


with open("README.md") as f:
    module_discription = f.read()

module = Extension(
    "pylua",
    sources=[
        "src/convert.c",
        "src/lua_py.c",
        "src/py_lua.c",
    ],
    include_dirs=[
        "src/",
        "/usr/include/lua5.4",
    ],
    libraries=[
        "lua5.4",
    ],
)

setup(
    name="pylua",
    version="0.0.1",
    description="General Purpose Binding between Python & lua",
    ext_modules=[module],
    author="Vipul Cariappa",
    author_email="vipulcariappa@gmail.com",
    url="https://github.com/Vipul-Cariappa/py-lua",
    long_description=module_discription,
)
