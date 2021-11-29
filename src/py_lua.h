#ifndef PY_LUA_H
#define PY_LUA_H

// stdlib
#include <assert.h>

#include <Python.h> // python

// lua
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// custom
#include "pylua.h"


// declaration in lua_py.c
extern lua_State* cL;	// TODO: Remove this from global state


// declaration in convert.c
PyObject* PyLua_LuaToPython(lua_State* L, int index);
int PyLua_PythonToLua(lua_State* L, PyObject* pItem);


// declaration here
PyTypeObject pLuaInstance_Type;
static PyObject* LuaError;


/* 
raise error to python interpreter with given msg
always returns NULL
*/   
static PyObject* raise_error(const char* msg)
{
	if (PyErr_Occurred())
	{
		// TODO:
		// if already error is occured
		// add a new frame with the following
		// PyErr_SetString(LuaError, msg);
		return NULL;
	}

	PyErr_SetString(LuaError, msg);
	return NULL;
}

/*
converts lua type to python type to return from a function
L is the lua_State and len is the number of return values
if len is more than 1 all return values is stored in tuple and returned
if len is 0 None is returned
*/
static PyObject* create_return(lua_State* L, int len)
{
	SAVE_STACK_SIZE(L);

	PyObject* pReturn = NULL;

	if (len == 0)
	{
		Py_INCREF(Py_None);
		pReturn = Py_None;
	}
	else if (len == 1)
	{
		pReturn = PyLua_LuaToPython(L, -1);
		if (!pReturn)
		{
			return raise_error(PERR_CONVERT_LUA_TO_PY);
		}
	}
	else
	{
		pReturn = PyTuple_New(len);
		int tmp = lua_gettop(L);

		for (int i = tmp - len, j = 0; i <= tmp; i++, j++)
		{
			PyObject* pItem = PyLua_LuaToPython(L, i);
			if (!pItem)
			{
				return raise_error(PERR_CONVERT_LUA_TO_PY);
			}
			PyTuple_SetItem(pReturn, j, pItem);
		}
	}

	CHECK_STACK_SIZE(L, 0);

	return pReturn;
}

static PyObject* flip_boolean(PyObject* ob)
{
	if (ob == Py_True)
	{
		Py_DECREF(ob);
		ob = Py_False;
		Py_INCREF(ob);
	}
	else
	{
		Py_DECREF(ob);
		ob = Py_True;
		Py_INCREF(ob);
	}

	return ob;
}



#endif // PY_LUA_H
