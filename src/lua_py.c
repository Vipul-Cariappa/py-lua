#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// bindings.c
int PyLua_PySet(lua_State* L);
int PyLua_PyGet(lua_State* L);
int PyLua_PyLoadModule(lua_State* L);
int PyLua_PyUnloadModule(lua_State* L);

static const struct luaL_Reg PY_lib[] = {
	{"PyLoad", PyLua_PyLoadModule},
	{"PyUnLoad", PyLua_PyUnloadModule},
	{NULL, NULL}
};

#if defined(_WIN32)
	__declspec(dllexport)
#endif
int luaopen_pylua(lua_State * L) {
	// python module
	luaL_newmetatable(L, "Python.Module");
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, PyLua_PyUnloadModule);
	lua_settable(L, -3);
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, PyLua_PyGet);
	lua_settable(L, -3);
	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, PyLua_PySet);
	lua_settable(L, -3);
	luaL_setfuncs(L, PY_lib, 0);
	lua_setglobal(L, "Python");

	return 1;
}
