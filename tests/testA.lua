lu = require('luaunit')


string = "Lua"
int = 121
boolean = false
list1 = {0, 1, 2, 3, 4, 5}
list2 = {1, 2, 3, 4, 5}


function return121()
    return 121
end

function get_values(a, b, c, d)
    lu.assertEquals(a, 121)
    lu.assertEquals(b, 1.5)
    lu.assertEquals(c, false)
    lu.assertEquals(d, "Python")
end

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
