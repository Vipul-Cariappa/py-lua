lu = require('luaunit')
require("pylua")


py_module = Python.PyLoad("test")


function test_python_get()
    local a = py_module.number
    local b = py_module.double
    local c = py_module.boolean
    local d = py_module.string
    lu.assertEquals(a, 121)
    lu.assertEquals(b, 1.5)
    lu.assertEquals(c, false)
    lu.assertEquals(d, "Python")
end


function test_pythoncallback()
    local x = py_module.callme
    local result = x(nil, true, "vipul", 12)
    lu.assertEquals(result, nil)
end

function test_lua_tableconvertion()
    local table = {
        a = "A1",
        b = "B1",
        c = "C1",
        d = "D1",
        e = "E1",
    }

    local array = {1, 2, 3, 4, 5, 6}
    py_module.callme(table, array)
end

-- os.exit(lu.LuaUnit.run())
lu.LuaUnit.run()
