lu = require('luaunit')
require("pylua")

py_module = Python.PyLoad("test1")

Rectangle = {length = 0, breadth = 0}

function Rectangle:new(length, breadth)
    o = {}
    setmetatable(o, self)
    o.__index = self
    o.length = length
    o.breadth = breadth
    return o
end

function Rectangle:get_area()
    self.area = self.length * self.breadth
    return self.area
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

function Call:__call(other)
    return "Call:_call function"
end


r1 = Rectangle:new(1, 2)
r2 = Rectangle:new(10, 20)

c = Call:new()


function test_passing_objects()
    py_module.pass_object(r1)
end


function test_add_objects()
    py_module.add_objects(r1, r2)
end

-- os.exit(lu.LuaUnit.run())
lu.LuaUnit.run()
