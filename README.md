# py-lua
![workflow](https://github.com/Vipul-Cariappa/py-lua/actions/workflows/build_and_test.yml/badge.svg)
[![codecov](https://codecov.io/gh/Vipul-Cariappa/py-lua/branch/main/graph/badge.svg?token=LOUBWQJGF9)](https://codecov.io/gh/Vipul-Cariappa/py-lua)

Bridge between Python and Lua.
This project has been started to provide easy binding between python and lua programming languages. Currently tested with lua version 5.4 and python.

## Currently py-lua supports:
- Importing python module into lua program.
- Getting string, float, boolean and None data types from python.
- Convertion between python dict and lua table.
- Calling and extracting return values of functions with simple data types.
- Working with python generators and lua threads.
- Support to use Object Oriented Programming between the languages.

## Example
### using python inside lua
```python
# mymodule.py
PI = 3.14
e = 2.71

def add(x, y):
    return x + y

def sub(x, y):
    return x - y

def concatenate(x, y):
    return x + y

class Rectangle:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def get_area(self):
        return self.x * self.y

    def __str__(self):
        return f"Rectangle(x={self.x}, y={self.y})"

```

```lua
-- main.lua
require("pylua")

pymodule = Python.PyLoad("mymodule")

print("12 + 13 = ", pymodule.add(12, 13))
-- 12 + 13 = 25
print("e - pi = ", pymodule.sub(pymodule.PI, pymodule.e))
-- 3.17 - 2.71 = 0.43
print(pymodule.concatenate("Lua loves ", "Python"))
-- Lua loves Python

rect = pymodule.Rectangle(4, 6)
print(rect.get_area())
-- 24
print(tostring(rect))
-- Rectangle(x=4.0, y=6.0)

```
### Using lua inside python
```lua
-- mymodule.lua

function celsius_to_fahrenheit(x)
    return (x * 1.8) + 32
end

function fahrenheit_to_celsius(x)
    return (x - 32) * .5556
end
```

```python
# main.py
import pylua
    
lua_module = pylua.LuaLoad("mymodule.lua")

ctf_40 = lua_module.celsius_to_fahrenheit(40)
ftc_n40 = lua_module.fahrenheit_to_celsius(-40)

print(f"{ctf_40 = }, {ftc_n40 = }") # ctf_40 = 104.0, ftc_n40 = -40.0032
```

## Building
To compile py-lua. First install the required dependencies:
- python3
- lua5.4
- cmake
- pkg-config

Clone the repository

```bash
git clone https://github.com/Vipul-Cariappa/py-lua.git
```

You should be able to build it with CMake

```bash
cmake . -DCMAKE_BUILD_TYPE=Release
```

Followed by `make` in Unix.

You will then need to copy `lib/libpylua.so` to your working directory as `pylua.so`.

> Building py-lua on Windows is not yet tested.

## Contribution
All contributions are welcomed. 
You can fork the project and create new PR, if you want to contribute. 
If the PR is related to a bud, creating a issue before creating the PR is recommended.

## Discussions
If you have any problems or have any feature request please feel free to post [new discussion](https://github.com/Vipul-Cariappa/py-lua/discussions/new). I will try to get at all of them.
