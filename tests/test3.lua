lu = require('luaunit')
require("pylua")

py_module = Python.PyLoad("test")


function test_python_error()
    lu.assertError(py_module.raise_error)
end

function func()
    local f = lol.x
end

function error_out()
    error("error")
end

function test_lua_error()
    lu.assertError(py_module.get_called_notprotected, func)
    lu.assertError(py_module.get_called_notprotected, error_out)
end

os.exit(lu.LuaUnit.run())
