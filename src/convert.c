#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// callable.c
void get_PyFunc(lua_State* L, PyObject* pFunc);
PyObject* get_LuaFunc(lua_State* L, int index);
PyObject* PyLua_pylua_module;


int PyLua_PythonToLua(lua_State* L, PyObject* pItem)
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
		get_PyFunc(L, pItem);
		return 1;
	}
	else if (PyDict_Check(pItem))
	{
		// to table
		PyObject* pKeyList = PyDict_Keys(pItem);

		lua_newtable(L);

		if (pKeyList)
		{
			PyObject* pKey, * pValue;

			Py_ssize_t len = PyList_Size(pKeyList);

			for (int i = 0; i < len; i++)
			{
				pKey = PyList_GetItem(pKeyList, i);
				pValue = PyDict_GetItem(pItem, pKey);

				PyLua_PythonToLua(L, pKey); // -1 => key
				PyLua_PythonToLua(L, pValue); // -1 => value  -2 => key

				lua_settable(L, -3);
			}
		}
		Py_DECREF(pKeyList);

		return 1;

	}
	else if (PyList_Check(pItem))
	{
		// to list using lua table
		lua_newtable(L);

		PyObject* pListElement;
		Py_ssize_t len = PyList_Size(pItem);

		for (int i = 0; i < len; i++)
		{
			pListElement = PyList_GetItem(pItem, i);
			PyLua_PythonToLua(L, pListElement);

			lua_seti(L, -2, i + 1);
		}

		return 1;
	}
	else if (PyTuple_Check(pItem))
	{
		// to list using lua table
		lua_newtable(L);

		PyObject* pTupleElement;
		Py_ssize_t len = PyTuple_Size(pItem);

		for (int i = 0; i < len; i++)
		{
			pTupleElement = PyTuple_GetItem(pItem, i);
			PyLua_PythonToLua(L, pTupleElement);

			lua_seti(L, -2, i + 1);
		}

		return 1;
	}
	else if (PySet_Check(pItem))
	{
		// to list using lua table
		lua_newtable(L);

		PyObject* pSetIter = PyObject_GetIter(pItem);
		PyObject* pSetElement;

		int i = 1;

		while ((pSetElement = PyIter_Next(pSetIter)))
		{
			PyLua_PythonToLua(L, pSetElement);

			lua_seti(L, -2, i);

			Py_DECREF(pSetElement);
			i++;
		}

		Py_DECREF(pSetIter);

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
	else if (lua_type(L, index) == LUA_TTABLE)
	{
		// Push another reference to the table on top of the stack (so we know
		// where it is, and this function can work for negative, positive and
		// pseudo indices
		lua_pushvalue(L, index);
		// stack now contains: -1 => table

		lua_pushnil(L);
		// stack now contains: -1 => nil; -2 => table

		// length of table
		//int len = luaL_len(L, index);

		pItem = PyDict_New();

		if (pItem)
		{
			PyObject* pKey;
			PyObject* pValue;

			while (lua_next(L, -2))
			{
				// stack now contains: -1 => value; -2 => key; -3 => table
				// copy the key so that lua_tostring does not modify the original
				lua_pushvalue(L, -2);

				// stack now contains: -1 => key; -2 => value; -3 => key; -4 => table
				pKey = PyLua_LuaToPython(L, -1);
				pValue = PyLua_LuaToPython(L, -2);
				PyDict_SetItem(pItem, pKey, pValue);
				Py_DECREF(pKey);
				Py_DECREF(pValue);

				// pop value + copy of key, leaving original key
				lua_pop(L, 2);
				// stack now contains: -1 => key; -2 => table
			}
			// stack now contains: -1 => table (when lua_next returns 0 it pops the key
			// but does not push anything.)
			// Pop table
			lua_pop(L, 1);
			// Stack is now the same as it was on entry to this function

			return pItem;

		}
	}
	else if (lua_type(L, index) == LUA_TFUNCTION)
	{
		uintptr_t prtvalue = L;

		PyObject* func = PyObject_GetAttrString(PyLua_pylua_module, "lua_function_wrapper");

		PyObject* pArgs = Py_BuildValue("(K)", prtvalue);

		return PyObject_CallObject(func, pArgs);

		//return get_LuaFunc(L, index);
	}

	return NULL;
}
