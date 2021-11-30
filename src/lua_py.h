#ifndef LUA_PY_H
#define LUA_PY_H

// stdlib
#include <assert.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>

// python
#include <Python.h>
#include <frameobject.h>

// lua
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// custom
#include "c_string.h"
#include "pylua.h"

#define LUA_MEMORY_ERROR(L) return luaL_error((L), MEMROY_ERR)
#define TRACEBACK_STR_LEN 256
#define EXCEPTION_STR_LEN 256


// declaration in py_lua.c
PyMODINIT_FUNC PyInit_pylua(void);


// declaration here
typedef PyObject* (*binary_op)(PyObject*, PyObject*);   // python binary operator function
typedef PyObject* (*unary_op)(PyObject*);   // python unary operator function
PyObject* pPylua_Module;    // python's pylua module
lua_State* cL;              // lua secondary state (thread)
static const struct luaL_Reg PY_lib[3];
static const struct luaL_Reg PY_Call_Wrapper[24];



/* raises error to lua interpreter with gien msg */
static int raise_error(lua_State* L, const char* msg)
{
	if (!PyErr_Occurred())
	{
		return luaL_error(L, msg);
	}

	size_t err_max = 1024;

	char* _err_msg = malloc(err_max);
	char* err_msg = _err_msg;

	if (!err_msg)
	{
		LUA_MEMORY_ERROR(L);
	}

	if ((err_msg += snprintf(err_msg, err_max, "%s\n\nPython Traceback:\n", msg)) < 0)

	{
		LUA_MEMORY_ERROR(L);
	}

	PyObject* pExcType;
	PyObject* pExcValue;
	PyObject* pExcTraceback;
	PyErr_Fetch(&pExcType, &pExcValue, &pExcTraceback);
	PyErr_NormalizeException(&pExcType, &pExcValue, &pExcTraceback);

	if (pExcTraceback && PyTraceBack_Check(pExcTraceback))
	{
		PyTracebackObject* pTrace = (PyTracebackObject*)pExcTraceback;

		char* traceback_msg = malloc(TRACEBACK_STR_LEN);
		if (!traceback_msg)
		{
			LUA_MEMORY_ERROR(L);
		}

		while (pTrace != NULL)
		{
			PyFrameObject* frame = pTrace->tb_frame;
			PyCodeObject* code = frame->f_code;

			int lineNr = PyFrame_GetLineNumber(frame);
			const char* codeName = PyUnicode_AsUTF8(code->co_name);
			const char* fileName = PyUnicode_AsUTF8(code->co_filename);

			if (snprintf(traceback_msg, TRACEBACK_STR_LEN, "File \"%s\", line %i, in\n  %s\n", fileName, lineNr, codeName) < 0)
			{
				LUA_MEMORY_ERROR(L);
			}

			if ((err_msg += snprintf(err_msg, err_max, "%s", traceback_msg)) < 0)
			{
				err_max *= 4;
				err_msg = err_msg - _err_msg;
				char* tmp = realloc(_err_msg, err_max);

				if (tmp)
				{
					_err_msg = tmp;
					err_msg = _err_msg + (long)err_msg;
				}
				else
				{
					LUA_MEMORY_ERROR(L);
				}

				if ((err_msg += snprintf(err_msg, err_max, "%s", traceback_msg)) < 0)
				{
					LUA_MEMORY_ERROR(L);
				}
			}

			pTrace = pTrace->tb_next;
		}

		free(traceback_msg);
	}

	PyObject* str_exc_value = PyObject_Repr(pExcValue);
	PyObject* pExcValueStr = PyUnicode_AsEncodedString(str_exc_value, "utf-8", "strict");
	const char* strExcValue = PyBytes_AS_STRING(pExcValueStr);

	char* err_name = malloc(EXCEPTION_STR_LEN);
	if (!err_name)
	{
		LUA_MEMORY_ERROR(L);
	}

	char* new_strExcValue = str_replace(strExcValue, "\\n", "\n");

	if (snprintf(err_name, EXCEPTION_STR_LEN, "\n  %s\n", new_strExcValue) < 0)
	{
		LUA_MEMORY_ERROR(L);
	}

	if (snprintf(err_msg, err_max, "%s", err_name) < 0)
	{
		err_max *= 4;
		err_msg = err_msg - _err_msg;
		char* tmp = realloc(_err_msg, err_max);

		if (tmp)
		{
			_err_msg = tmp;
			err_msg = _err_msg + (long)err_msg;
		}
		else
		{
			LUA_MEMORY_ERROR(L);
		}

		if ((err_msg += snprintf(err_msg, err_max, "%s", err_name)) < 0)
		{
			LUA_MEMORY_ERROR(L);
		}
	}

	lua_pushstring(L, _err_msg);

	Py_XDECREF(pExcType);
	Py_XDECREF(pExcValue);
	Py_XDECREF(pExcTraceback);

	Py_XDECREF(str_exc_value);
	Py_XDECREF(pExcValueStr);

	free(_err_msg);
	free(err_name);
	free(new_strExcValue);

	return lua_error(L);
}

/*
performs the binary_op on to the top two variables from the lua state 
by converting them to python types 
and pushes the return value
*/
static int binary_base_pyobj_wrapper(lua_State* L, binary_op func)
{
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, 1);

	PyObject* other = PyLua_LuaToPython(L, 2);
	if (!other)
	{
		return raise_error(L, LERR_CONVERT_LUA_TO_PY);
	}

	PyObject* pReturn = func(py_obj->object, other);

	Py_DECREF(other);

	if (pReturn)
	{
		if (PyLua_PythonToLua(L, pReturn) < 0)
		{
			Py_DECREF(pReturn);
			return raise_error(L, LERR_CONVERT_LUA_TO_PY);
		}
		Py_DECREF(pReturn);
		return 1;
	}

	return raise_error(L, ERR_CALL_PY);
}



// Python Wrapper functions

int call_PyFunc(lua_State* L);
int iter_PyGenerator(lua_State* L);
static int get_PyObj(lua_State* L);
static int set_PyObj(lua_State* L);
static int str_PyObj(lua_State* L);
static int call_PyObj(lua_State* L);
static int gc_PyObj(lua_State* L);
static int add_PyObj(lua_State* L);
static int sub_PyObj(lua_State* L);
static int mul_PyObj(lua_State* L);
static int div_PyObj(lua_State* L);
static int floordiv_PyObj(lua_State* L);
static int pow_PyObj(lua_State* L);
static int mod_PyObj(lua_State* L);
static int lshift_PyObj(lua_State* L);
static int rshift_PyObj(lua_State* L);
static int band_PyObj(lua_State* L);
static int bor_PyObj(lua_State* L);
static int bxor_PyObj(lua_State* L);
static int eq_PyObj(lua_State* L);
static int lt_PyObj(lua_State* L);
static int le_PyObj(lua_State* L);
static int len_PyObj(lua_State* L);
static int neg_PyObj(lua_State* L);
static int bnot_PyObj(lua_State* L);


// Lua Module Functions

static int PyLua_PyLoadModule(lua_State* L);
static int PyLua_PyUnloadModule(lua_State* L);
static int PyLua_PySet(lua_State* L);
static int PyLua_PyGet(lua_State* L);


#endif // LUA_PY_H
