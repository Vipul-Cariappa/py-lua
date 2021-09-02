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

function test_lua_table_convertion()
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

function test_python_dict_convertion()

    local py_table = py_module.table

    local table = {
        a="AA",
        b="BB",
        c="CC",
        d="DD",
    }
    table[10.0] = 10.0

    lu.assertEquals(table, py_table)

    -- for k,v in pairs(table) do
    --     print(k, " => ",v)
    -- end

end

function test_python_list_conversion()
    local py_list = py_module.pyList
    lu.assertEquals({"List", 12}, py_list)
end

function test_python_tuple_conversion()
    local py_tuple = py_module.pyTuple
    lu.assertEquals({"Tuple", 12}, py_tuple)
end

function test_python_set_conversion()
    local py_set = py_module.pySet    
    -- lu.assertEquals({"Set", 12}, py_set)
    lu.assertEquals({12, "Set"}, py_set)
end

function nothing()
    print("From Lua Function")
    return nil
end

function test_lua_function_convert()
    py_module.callme(nothing)
end

-- os.exit(lu.LuaUnit.run())
lu.LuaUnit.run()
