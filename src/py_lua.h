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
static PyTypeObject pLuaFunc_Type;
PyTypeObject pLuaInstance_Type;		// used elsewhere
PyTypeObject pLuaTable_Type;		// used elsewhere
static PyObject* LuaError;
static struct PyModuleDef LUA_module;

static PyMappingMethods pLuaInstance_MappingMethods;
static PyMappingMethods pLuaTable_MappingMethods;
static PyNumberMethods pLuaInstance_NumberMethods;


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

/* returns Py_True if Py_False is passed in or opposite */
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

/*
performs binary operation on self and other with op as the operand
example: self + other if op is __add
*/
static PyObject* operation_LuaTable_base(PyLua_LuaTable* self, PyObject* other, const char* op)
{
	PyObject* pReturn;

	// get table
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_pushnil(cL); // placeholder for function (method)
	lua_rawgeti(cL, -2, self->index);

	// get function (method)
	lua_getfield(cL, -1, op);
	lua_replace(cL, -3);

	// get other element
	if (PyLua_PythonToLua(cL, other) < 0)
	{
		return raise_error(PERR_CONVERT_PY_TO_LUA);
	}

	// call function
	if (lua_pcall(cL, 2, 1, 0) != LUA_OK)
	{
		//TODO: manage stack before raising error

		PyErr_Format(LuaError, ERR_CALL_LUA, lua_tostring(cL, -1));
		return NULL;
	}

	pReturn = PyLua_LuaToPython(cL, -1);
	
	if (!pReturn)
	{
		return raise_error(PERR_CONVERT_LUA_TO_PY);
	}

	lua_pop(cL, 2);	// result and registry

	CHECK_STACK_ZERO(cL);

	return pReturn;
}

/* executer for wrapped lua method (function associated to table) */
static int wrapper_around_method(lua_State* L)
{
	int stack_size = lua_gettop(L);
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_insert(L, 1);
	lua_pushvalue(L, lua_upvalueindex(2));
	lua_insert(L, 1);

	assert(stack_size + 2 == lua_gettop(L));

	// execute function
	if (lua_pcall(L, stack_size + 1, LUA_MULTRET, 0) != LUA_OK)
	{
		// TODO: to test proper error handling; add test which raises error from lua method
		// PyErr_Format(LuaError, ERR_CALL_LUA, lua_tostring(cL, -1));
		// return NULL;
		return lua_error(cL);
	}

	int return_len = lua_gettop(L);
	assert(lua_gettop(L) == return_len);

	return return_len;
}


// Lua function and thread wrapper functions

static PyObject* init_LuaFunc_Wrapper(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs);
static PyObject* call_LuaFunc(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs);
static PyObject* iter_LuaCoroutine(PyLua_LuaFunc* self);
static PyObject* next_LuaCoroutine(PyLua_LuaFunc* self);
static void gc_LuaFunc(PyLua_LuaFunc* self);


// Lua table wrapper functions

static int init_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* args, PyObject* kwargs);
static PyObject* getelem_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* pKey);
static int setelem_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* pKey, PyObject* pValue);
static PyObject* call_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* args, PyObject* kwargs);
static void gc_LuaTable(PyLua_LuaTable* self);


// Lua instance (table) wrapper functions

static PyObject* getattr_LuaInstance_Wrapper(PyLua_LuaTable* self, char* attr);
static int setattr_LuaInstance_Wrapper(PyLua_LuaTable* self, char* attr, PyObject* pValue);
static PyObject* call_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* args, PyObject* kwargs);
static PyObject* string_LuaInstance_Wrapper(PyLua_LuaTable* self);
static PyObject* concat_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other);
static PyObject* add_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other);
static PyObject* sub_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other);
static PyObject* mul_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other);
static PyObject* div_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other);
static PyObject* pow_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other, PyObject* t);
static PyObject* mod_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other);
static PyObject* floordiv_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other);
static PyObject* band_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other);
static PyObject* bor_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other);
static PyObject* bxor_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other);
static PyObject* lshift_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other);
static PyObject* rshift_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other);
static PyObject* bnot_LuaInstance_Wrapper(PyLua_LuaTable* self);
static PyObject* neg_LuaInstance_Wrapper(PyLua_LuaTable* self);
static PyObject* compare_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other, int op);
static Py_ssize_t len_LuaInstance_Wrapper(PyLua_LuaTable* self);

#endif // PY_LUA_H
