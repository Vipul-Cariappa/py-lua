lu = require('luaunit')
require("pylua")

py_module = Python.PyLoad("test1")

Rectangle = {length = 0, breadth = 0}

function Rectangle:new(length, breadth)
    o = {}
    setmetatable(o, self)
    self.__index = self
    o.length = length
    o.breadth = breadth
    return o
end

function Rectangle:__tostring()
    return "Rectangle(length = " .. tostring(self.length) .. ", Breadth = " .. tostring(self.breadth) .. ")"
end

function Rectangle:get_area()
    self.area = self.length * self.breadth
    return self.area
end

function Rectangle:other(a, b, c)
    lu.assertEquals(a, 2)
    lu.assertEquals(b, 3)
    lu.assertEquals(c, false)
end

function Rectangle:__add(other)
    if type(other) == "table" then
        return Rectangle:new(self.length + other.length, self.breadth + other.breadth)
    end
    return Rectangle:new(self.length + other, self.breadth + other)
end

function Rectangle:__sub(other)
    if type(other) == "table" then
        return Rectangle:new(self.length - other.length, self.breadth - other.breadth)
    end
    return Rectangle:new(self.length - other, self.breadth - other)
end

function Rectangle:__mul(other)
    return Rectangle:new(self.length * other, self.breadth * other)
end

function Rectangle:__div(other)
    return self * (1/other)
end

function Rectangle:__unm()
    return Rectangle:new(self.length * -1, self.breadth * -1)
end

function Rectangle:__bnot()
    return -1
end

function Rectangle:__len()
    return 0
end

function Rectangle:__eq(other)
    if self.lenght == other.length and self.breadth == other.breadth then
        return true
    end
    return false
end

function Rectangle:__lt(other)
    local self_area = self:get_area()
    local other_area = other:get_area()

    if self_area < other_area then
        return true
    end
    return false
end

function Rectangle:__le(other)
    local self_area = self:get_area()
    local other_area = other:get_area()

    if self_area <= other_area then
        return true
    end
    return false
end


Call = {}
function Call:new()
    o = {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function Call:__call()
    return "Call:_call function"
end

function Call:__concat()
    return "Call:_concat function"
end


r1 = Rectangle:new(1, 2)
r2 = Rectangle:new(10, 20)

c = Call:new()


function test_passing_objects()
    py_module.pass_object(r1)
end


function test_operations_objects()
    py_module.operations_on_objects(r1, r2)
end

function test_setattr_python()
    lu.assertEquals(c.python, nil)
    py_module.insert_new(c)
    lu.assertEquals(c.python, "New Object")
end

function test_call_table_from_python()
    lu.assertEquals(py_module.handle_call(c), "Call:_call function")
end

function test_renamethistest()
    lu.assertEquals(
        tostring(py_module.return_me(r1)), 
        "Rectangle(length = 1, Breadth = 2)"
    )
end

function test_classes_lua()
    py_module.test_lua_class(Rectangle)
end

function test_python_operations()
    local tmp = py_module.c1 + py_module.c2
    lu.assertEquals(tmp.radius, 3)
    
    tmp = py_module.c1 - py_module.c2
    lu.assertEquals(tmp.radius, -1)
    
    tmp = py_module.c1 * 5
    lu.assertEquals(tmp.radius, 5)
    
    tmp = py_module.c2 ~= py_module.c1
    lu.assertEquals(tmp, true)
    
    tmp = py_module.c2 == py_module.c1
    lu.assertEquals(tmp, false)
    
    tmp = py_module.c2 < py_module.c1
    lu.assertEquals(tmp, false)

    tmp = tostring(py_module.c1)
    lu.assertEquals(tmp, "Circle(radius=1)")

end

function test_classes_python()
    local tmp = py_module.Circle(10)
    lu.assertEquals(tmp.radius, 10)
    lu.assertEquals(tmp.get_area(), 314)
    lu.assertEquals(tmp.other(1, 2), nil)
end

function test_python_method()
    lu.assertEquals(py_module.c1.get_area(), 3.14)
    lu.assertEquals(py_module.c2.get_area(), 12.56)
end

function test_python_call()
    lu.assertEquals(py_module.f(), "Call.__call__ function")
end

os.exit(lu.LuaUnit.run())
