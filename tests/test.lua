lu = require('luaunit')
require("pylua")

function test_pyadd()
    lu.assertEquals(add(2, 6), 8)
    lu.assertEquals(add(2, -6), -4)
end

function test_pysub()
    lu.assertEquals(sub(6, 2), 4)
    lu.assertEquals(sub(2, -6), 8)
end

function test_array()
    a = array.new(10)
    array.set(a, 5, 5)
    a:set(2, 2)
    lu.assertEquals(array.size(a), 10)
    lu.assertEquals(array.get(a, 5), 5)
    lu.assertEquals(a:get(2), 2)
end

function test_python()
    py_test_module = Python.PyLoad("test")
    num = py_test_module.number
    lu.assertEquals(num, 121.55)
end

function test_pythoncallback()
    x = py_test_module.callme
    x()
end

-- py_print()

-- os.exit(lu.LuaUnit.run())
lu.LuaUnit.run()