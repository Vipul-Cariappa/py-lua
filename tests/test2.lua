lu = require('luaunit')
require("pylua")

py_module = Python.PyLoad("test")

function co()
    coroutine.yield(1)
    coroutine.yield(2)
    coroutine.yield(3)
end

function test_coroutine1()
    c1 = coroutine.create(co)
    py_module.handle_iter(c1)
end

function test_coroutine2()
    c2 = coroutine.create(co)
    py_module.handle_iter_all(c2)
end

function test_py_coroutine1()
    local range = py_module.c_range(2)
    
    local _, x = coroutine.resume(range)
    lu.assertEquals(x, 0)

    local _, x = coroutine.resume(range)
    lu.assertEquals(x, 1)
    
    local _, x = coroutine.resume(range)
    lu.assertEquals(_, false)

    local _, x = coroutine.resume(range)
    lu.assertEquals(_, false)
end


function test_py_coroutine2()
    local range = py_module.c_range(-1)
    local _, x = coroutine.resume(range)
    lu.assertEquals(_, false)
end


-- os.exit(lu.LuaUnit.run())
lu.LuaUnit.run()
