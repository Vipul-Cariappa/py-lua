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


-- os.exit(lu.LuaUnit.run())
lu.LuaUnit.run()
