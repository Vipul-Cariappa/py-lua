#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// callable.c
void get_PyFunc(lua_State* L, PyObject* pFunc, PyObject* pModule);

int PyLua_PythonToLua(lua_State* L, PyObject* pItem, PyObject* pModule)
{
	if (pItem == Py_True)
	{
		// to boolean
		lua_pushboolean(L, 1);

		return 1;
	}
	else if (pItem == Py_False)
	{
		// to boolean
		lua_pushboolean(L, 0);

		return 1;
	}
	else if (PyNumber_Check(pItem))
	{
		// to double
		double result = PyFloat_AsDouble(pItem);
		lua_pushnumber(L, result);

		return 1;
	}
	else if (PyUnicode_Check(pItem))
	{
		// to string
		PyObject* encodedString = PyUnicode_AsEncodedString(pItem, "UTF-8", "strict");
		if (encodedString)
		{
			const char* result = PyBytes_AsString(encodedString);
			if (result) {
				lua_pushstring(L, result);
			}
			Py_DECREF(encodedString);
		}

		return 1;
	}
	else if (pItem == Py_None)
	{
		// to function
		lua_pushnil(L);

		return 1;
	}
	else if (PyCallable_Check(pItem))
	{
		// to function
		get_PyFunc(L, pItem, pModule);
		return 1;
	}

	return -1;
}

PyObject* PyLua_LuaToPython(lua_State* L, int index)
{
	PyObject* pItem;

	if (lua_type(L, index) == LUA_TNUMBER)
	{
		// to float
		double x = lua_tonumber(L, index);
		pItem = PyFloat_FromDouble(x);

		return pItem;
	}
	else if (lua_type(L, index) == LUA_TNIL)
	{
		return Py_None;
	}
	else if (lua_type(L, index) == LUA_TBOOLEAN)
	{
		int x = lua_toboolean(L, index);
		if (x)
		{
			Py_RETURN_TRUE;
		}
		Py_RETURN_FALSE;
	}
	else if (lua_type(L, index) == LUA_TSTRING)
	{
		const char* x = lua_tostring(L, index);
		pItem = PyUnicode_FromString(x);
		return pItem;
	}

	return NULL;
}
