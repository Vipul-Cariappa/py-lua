#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef struct PY_callable {
	PyObject* module;
	PyObject* function;
} PY_callable;


int PYCallable_unload(lua_State* L)
{
	return 0;
}

int call(lua_State* L)
{
	printf("You are calling the c call function\n");
	return 0;
}

void get_pylua_func(lua_State* L, PyObject* pyfunc, PyObject* pymodule)
{
	

	// creating new lua python callable
	size_t nbytes = sizeof(PY_callable);
	PY_callable* luapy_callable = (PY_callable*)lua_newuserdata(L, nbytes);
	luapy_callable->function = pyfunc;
	luapy_callable->module = pymodule;
	luaL_getmetatable(L, "Python.Callable");
	lua_setmetatable(L, -2);

	// getting the call function
	int result = lua_getfield(L, -1, "call");
	lua_remove(L, -2);
	if (result == LUA_TFUNCTION)
	{
		return;
	}

	return luaL_error(L, "Some Internal Error Occurred");
}
