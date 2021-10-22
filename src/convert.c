#include "pylua.h"


// lua_py.c
PyObject* pPylua_Module;
PyTypeObject pLuaInstance_Type;
PyTypeObject pLuaTable_Type;
int call_PyFunc(lua_State* L);
int iter_PyGenerator(lua_State* L, ...);


int PyLua_PythonToLua(lua_State* L, PyObject* pItem)
{
	SAVE_STACK_SIZE(L);

	if (pItem == Py_True)
	{
		// to boolean
		lua_pushboolean(L, 1);

	}
	else if (pItem == Py_False)
	{
		// to boolean
		lua_pushboolean(L, 0);

	}
	else if (PyNumber_Check(pItem))
	{
		// to double
		double result = PyFloat_AsDouble(pItem);
		lua_pushnumber(L, result);

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

	}
	else if (pItem == Py_None)
	{
		// to nil
		lua_pushnil(L);

	}
	else if (PyFunction_Check(pItem) || PyMethod_Check(pItem))
	{
		// creating new lua function
		size_t nbytes = sizeof(PyLua_PyFunc);
		PyLua_PyFunc* py_callable = (PyLua_PyFunc*)lua_newuserdata(L, nbytes);

		py_callable->function = pItem;

		lua_pushcclosure(L, call_PyFunc, 1);

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

			lua_seti(L, -2, (lua_Integer)i + 1);
		}

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

			lua_seti(L, -2, (lua_Integer)i + 1);
		}

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
	else if (PyIter_Check(pItem))
	{
		// creating new lua thread

		lua_State* n = lua_newthread(L);

		size_t nbytes = sizeof(PyLua_PyIterator);
		PyLua_PyIterator* py_iter = (PyLua_PyIterator*)lua_newuserdata(n, nbytes);

		PyObject* iter = PyObject_GetIter(pItem);

		if (!iter)
		{
			PyErr_Print();
			return luaL_error(L, "Error: Memory error");
		}

		py_iter->iterator = iter;

		lua_pushcclosure(n, iter_PyGenerator, 1);

	}
	else
	{
		// assume class or instance

		// check if python object is wrapper around lua table
		if (PyObject_IsInstance(pItem, (PyObject*)&pLuaInstance_Type) || PyObject_IsInstance(pItem, (PyObject*)&pLuaTable_Type))
		{			
			lua_pushnil(L);
			lua_pushvalue(L, LUA_REGISTRYINDEX);
			lua_geti(L, -1, ((PyLua_LuaTable*)pItem)->index);
			lua_replace(L, -3);
			lua_pop(L, 1);
		}

		lua_newtable(L);
		lua_getglobal(L, "PythonClassWrapper");
		lua_setmetatable(L, -2);

		size_t nbytes = sizeof(PyLua_PyObject);
		PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_newuserdata(L, nbytes);
		lua_setfield(L, -2, "__python");

		py_obj->object = pItem;
	}

	CHECK_STACK_SIZE(L, 1);

	return 1;
}

PyObject* PyLua_LuaToPython(lua_State* L, int index)
{
	SAVE_STACK_SIZE(L);

	PyObject* pReturn = NULL;
	int type = lua_type(L, index);

	if (type == LUA_TNUMBER)
	{
		// to float
		double x = lua_tonumber(L, index);
		pReturn = PyFloat_FromDouble(x);

	}
	else if (type == LUA_TNIL)
	{
		// to none
		Py_INCREF(Py_None);
		pReturn = Py_None;

	}
	else if (type == LUA_TBOOLEAN)
	{
		// to boolean
		int x = lua_toboolean(L, index);
		if (x)
		{
			Py_INCREF(Py_True);
			pReturn = Py_True;
		}
		Py_INCREF(Py_False);
		pReturn = Py_False;

	}
	else if (type == LUA_TSTRING)
	{
		// to string
		const char* x = lua_tostring(L, index);
		pReturn = PyUnicode_FromString(x);

	}
	else if (type == LUA_TTABLE)
	{
		// check if table is a wrapper around python object
		lua_getfield(L, index, "__python");
		if (lua_type(L, -1) == LUA_TUSERDATA)
		{
			PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
			lua_pop(L, 1);
			pReturn = py_obj->object;
		}
		lua_pop(L, 1);

		if (lua_getmetatable(L, index))
		{
			// to python instance
			
			lua_pop(L, 1); // remove metatable

			lua_pushvalue(L, LUA_REGISTRYINDEX);
			lua_pushvalue(L, index);
			int ref_value = luaL_ref(L, -2);

			lua_pop(L, 1);

			PyObject* obj = PyObject_GetAttrString(pPylua_Module, "lua_instance_wrapper");

			if (obj)
			{
				PyObject* pArgs = Py_BuildValue("(i)", ref_value);
				pReturn = PyObject_CallObject(obj, pArgs);

			}
			
			//TODO: improve error handling
			//if (PyErr_Occurred())
			//{
			//	PyErr_Print();
			//}
			//lua_pop(L, 1);
			//return luaL_error(L, "Error: While executing python function");

		}
		else
		{
			lua_pushvalue(L, LUA_REGISTRYINDEX);
			lua_pushvalue(L, index);
			int ref_value = luaL_ref(L, -2);

			lua_pop(L, 1);

			PyObject* obj = PyObject_GetAttrString(pPylua_Module, "lua_table_wrapper");

			if (obj)
			{
				PyObject* pArgs = Py_BuildValue("(i)", ref_value);
				pReturn = PyObject_CallObject(obj, pArgs);

			}

			//TODO: improve error handling
			//if (PyErr_Occurred())
			//{
			//	PyErr_Print();
			//}
			//lua_pop(L, 1);
			//return luaL_error(L, "Error: While executing python function");
		}

	}
	else if (type == LUA_TFUNCTION)
	{
		// to python function
		lua_pushvalue(L, LUA_REGISTRYINDEX);
		lua_pushvalue(L, index);
		int ref_value = luaL_ref(L, -2);

		lua_pop(L, 1);

		PyObject* func = PyObject_GetAttrString(pPylua_Module, "lua_function_wrapper");

		if (func)
		{
			PyObject* pArgs = Py_BuildValue("(ii)", ref_value, 0);
			pReturn = PyObject_CallObject(func, pArgs);

		}
		
		//TODO: improve error handling
		//if (PyErr_Occurred())
		//{
		//	PyErr_Print();
		//}

		//return luaL_error(L, "Error: While executing python function");

	}
	else if (type == LUA_TTHREAD)
	{
		// to python generator
		lua_pushvalue(L, LUA_REGISTRYINDEX);
		lua_pushvalue(L, index);
		int ref_value = luaL_ref(L, -2);

		lua_pop(L, 1);

		PyObject* func = PyObject_GetAttrString(pPylua_Module, "lua_function_wrapper");

		if (func)
		{
			PyObject* pArgs = Py_BuildValue("(ii)", ref_value, 1);
			pReturn = PyObject_CallObject(func, pArgs);

		}
		
		//TODO: improve error handling
		//if (PyErr_Occurred())
		//{
		//	PyErr_Print();
		//}
		//return luaL_error(L, "Error: While executing python function");
	}

	assert(pReturn);

	CHECK_STACK_SIZE(L, 0);
}
