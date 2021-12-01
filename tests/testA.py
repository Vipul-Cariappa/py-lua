import unittest
import pylua

lua_module = pylua.LuaLoad("testA.lua")

class TestBasic(unittest.TestCase):
    def test_simple_types(self):
        self.assertEqual(lua_module.string, "Lua")
        self.assertEqual(lua_module.int, 121)
        self.assertFalse(lua_module.boolean)

    @unittest.skip("Convertion to list or iteration not yet implemented")
    def test_complex_types(self):
        for i in enumerate(lua_module.list):
            self.assertEqual(e, i)

    def test_function_calls(self):
        self.assertEqual(lua_module.return121(), 121)
        lua_module.get_values(121, 1.5, False, "Python")


if __name__ == "__main__":
    unittest.main()
