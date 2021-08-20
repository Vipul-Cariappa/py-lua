# py-lua
Bridge between Python and Lua.
This project has been started to provide easy binding between python and lua programming languages. Currently tested with lua version 5.3 and 5.4 and python version 3.8.

### Currently py-lua supports:
- Importing python module into lua program.
- Getting string, float, boolean and None data types from python.
- Calling and extracting return values of functions with simple data types.

## Example
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
```

## Building
To compile py-lua in linux. First install the required dependencies:
- python3-dev
- lua5.3
- liblua5.3-dev
- gcc

In ubuntu run

```bash
sudo apt-get install python3-dev lua5.3 liblua5.3-dev gcc -y
```

Clone the repository

```bash
git clone https://github.com/Vipul-Cariappa/py-lua.git
```

Build using gcc

```bash 
cd py-lua/src

gcc -fPIC -shared -o pylua.so bindings.c callable.c convert.c lua_py.c $(pkg-config --cflags --libs lua5.3) $(pkg-config --cflags --libs python3)
```

Copy the shared library pylua.so file to the working directory of your project.

If you face any problems while building please ask for help [here](https://github.com/Vipul-Cariappa/py-lua/discussions/new).


## Yet to Implement
- [x] Simple data type convertions
- [x] Calling python functions from lua
- [ ] Support list, tuple, dict and set
- [ ] Support for working with python classes from lua
- [ ] Support for generator functions
- [ ] Calling lua from python (lua bindings for python)

## Contribution
Any contribution are welcomed. 
You can fork the project and create new PR, if you want to contribute. 
If the PR is related to a bud, creating new issue before creating new PR is recommend.

## Discussions
If you have any problems or have any feature request please feel free to post [new discussion](https://github.com/Vipul-Cariappa/py-lua/discussions/new). I will try to get at all of them.
