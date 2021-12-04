import unittest
import pylua

lua_module = pylua.LuaLoad("testA.lua")

class TestBasic(unittest.TestCase):
    def test_simple_types(self):
        self.assertEqual(lua_module.string, "Lua")
        self.assertEqual(lua_module.int, 121)
        self.assertFalse(lua_module.boolean)

    @unittest.skip("Iteration not implemented")
    def test_complex_types(self):
        for i, j in lua_module.list1:
            self.assertEqual(i + 1, j)
        
        for i, j in lua_module.list2:
            self.assertEqual(i, j)

    def test_function_calls(self):
        self.assertEqual(lua_module.return121(), 121)
        lua_module.get_values(121, 1.5, False, "Python")

    def test_classes_and_instance(self):
        r1 = lua_module.Rectangle(4, 6)
        self.assertEqual(r1.get_area(), 24)
        self.assertEqual(r1.length, 4)
        self.assertEqual(r1.breadth, 6)
        self.assertEqual(str(r1), "Rectangle(length = 4.0, Breadth = 6.0)")



if __name__ == "__main__":
    unittest.main()
