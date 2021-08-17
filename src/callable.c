#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// convert.c
PyObject* PyLua_LuaToPython(lua_State* L, int index);
int PyLua_PythonToLua(lua_State* L, PyObject* pItem, PyObject* pModule);


typedef struct PyLua_PyCallable {
	PyObject* module;
	PyObject* function;
} PyLua_PyCallable;


int PyLua_PyCallFunc(lua_State* L)
{
	int args_count = lua_gettop(L);
	PyLua_PyCallable* luapy_callable = (PyLua_PyCallable*)lua_touserdata(L, lua_upvalueindex(1));

	PyObject* pArgs = PyTuple_New(args_count);
	PyObject* pItem;

	if (pArgs)
	{
		for (int i = 0, j = 1; i < args_count; i++, j++)
		{
			pItem = PyLua_LuaToPython(L, j);
			PyTuple_SetItem(pArgs, i, pItem);
		}

		PyObject* pResult = PyObject_CallObject(luapy_callable->function, pArgs);
		Py_DECREF(pArgs);
		if (pResult)
		{
			PyLua_PythonToLua(L, pResult, luapy_callable->module);
			Py_DECREF(pResult);
			return 1;
		}

		return luaL_error(L, "Error: Some Internal Problem");
	}

	return luaL_error(L, "Error: Some Internal Problem");
}


void get_PyFunc(lua_State* L, PyObject* pFunc, PyObject* pModule)
{
	// creating new lua python callable
	size_t nbytes = sizeof(PyLua_PyCallable);
	PyLua_PyCallable* py_callable = (PyLua_PyCallable*)lua_newuserdata(L, nbytes);
	py_callable->function = pFunc;
	py_callable->module = pModule;

	Py_INCREF(pFunc);
	Py_INCREF(pModule);

	lua_pushcclosure(L, PyLua_PyCallFunc, 1);
}
