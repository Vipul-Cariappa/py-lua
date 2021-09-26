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
    range1 = py_module.c_range(6)
    range2 = py_module.c_range(2)
    
    print(coroutine.resume(range1))
    print(coroutine.resume(range1))
    print(coroutine.resume(range1))
    print(coroutine.resume(range1))
    print(coroutine.resume(range1))
    print(coroutine.resume(range1))
    print(coroutine.resume(range1))
    print(coroutine.resume(range1))
    print(coroutine.resume(range2))
    print(coroutine.resume(range2))
    print(coroutine.resume(range2))
    print(coroutine.resume(range2))
    -- print(x)
end

-- os.exit(lu.LuaUnit.run())
lu.LuaUnit.run()
