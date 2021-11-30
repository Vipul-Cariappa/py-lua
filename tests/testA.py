import unittest
import pylua

lua_module = pylua.LuaLoad("testA")

class TestBasic(unittest.TestCase):
    def test_simple_types(self):
        self.assertEqual(lua_module.string, "Lua")
        self.assertEqual(lua_module.int, 121)
        self.assertFalse(lua_module.boolean)

    def test_complex_types(self):
        for i in enumerate(lua_module.list):
            self.assertEqual(e, i)

if __name__ == "__main__":
    unittest.main()
