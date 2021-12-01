lu = require('luaunit')


string = "Lua"
int = 121
boolean = false
list = {0, 1, 2, 3, 4, 5}


function return121()
    return 121
end

function get_values(a, b, c, d)
    lu.assertEquals(a, 121)
    lu.assertEquals(b, 1.5)
    lu.assertEquals(c, false)
    lu.assertEquals(d, "Python")
end
