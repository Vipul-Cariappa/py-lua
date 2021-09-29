lu = require('luaunit')
require("pylua")

py_module = Python.PyLoad("test")


function test_python_error()
    lu.assertError(py_module.raise_error)
    -- py_module.raise_error()
end

function func()
    local f = lol.x
end

function test_lua_error()
    lu.assertError(py_module.get_called_notprotected, func)
end

-- os.exit(lu.LuaUnit.run())
lu.LuaUnit.run()
