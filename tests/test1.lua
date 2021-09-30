lu = require('luaunit')
require("pylua")

py_module = Python.PyLoad("test0")

Python.PyUnLoad(py_module)

function error_out()
    local x = py_module.string
end

function test_errors()
    lu.assertError(Python.PyUnLoad, py_module)
    lu.assertError(error_out)
end

-- os.exit(lu.LuaUnit.run())
lu.LuaUnit.run()
