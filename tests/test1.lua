lu = require('luaunit')

require("pylua")

py = Python.PyLoad("test")

Python.PyUnLoad(py)

function error_out()
    local x = py.string
end

function test_errors()
    lu.assertError(Python.PyUnLoad, py)
    lu.assertError(error_out)
end

-- os.exit(lu.LuaUnit.run())
lu.LuaUnit.run()
