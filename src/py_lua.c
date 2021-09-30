#include "pylua.h"


typedef struct PyLua_LuaFunc {
	PyObject_HEAD
	void* lStack_prt;
	void* lFunc_prt;
	int is_luathread;
	int thread_terminated;
} PyLua_LuaFunc;


static PyObject* LuaError;


static PyObject* create_return(lua_State* L, int len)
{
	PyObject* pReturn = NULL;

	if (len == 0)
	{
		Py_INCREF(Py_None);
		pReturn = Py_None;
	}
	else if (len == 1)
	{
		pReturn = PyLua_LuaToPython(L, -1);
	}
	else
	{
		pReturn = PyTuple_New(len);
		int tmp = lua_gettop(L);

		for (int i = tmp - len, j = 0; i <= tmp; i++, j++)
		{
			PyTuple_SetItem(pReturn, j, PyLua_LuaToPython(L, i));
		}
	}

	return pReturn;
}

static PyObject* next_LuaCoroutine(PyLua_LuaFunc* self)
{
	PyObject* pReturn = NULL;

	if (self->thread_terminated)
	{
		PyErr_SetNone(PyExc_StopIteration);
		return NULL;
	}

	lua_State* L = (lua_State*)self->lStack_prt;

	// make sure size of stact is the same at exit
	int stack_size = lua_gettop(L);

	// error message if function is not found
	int found_func = 0;

	// get list of variables from lua
	lua_getglobal(L, "_G");

	lua_pushnil(L);

	// iterate over them to find the function
	while (lua_next(L, -2))
	{
		if (lua_topointer(L, -1) == self->lFunc_prt)
		{
			// found the function
			found_func = 1;

			// to find number of return values
			int current_stack = lua_gettop(L);

			// copy function
			lua_pushvalue(L, -1);

			// number of yield values
			int yield_count = 0;

			lua_State* co = lua_tothread(L, -1);

			// call the function
			int resume_result = lua_resume(co, L, 0, &yield_count);

			if (resume_result == LUA_YIELD)
			{
				pReturn = create_return(co, yield_count);
				lua_pop(co, yield_count);

			}
			else if (resume_result == LUA_OK)
			{
				self->thread_terminated = 1;

				pReturn = create_return(co, yield_count);
				lua_pop(co, yield_count);

			}
			else
			{
				//return luaL_error(L, "Error: While calling the lua function.");

				PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(L, -1));
				return NULL;
			}
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}

	lua_pop(L, 1); // remove _G

	if (!found_func)
	{
		//return luaL_error(L, "Error: Lua function not found");

		if (self->is_luathread)
		{
			PyErr_SetString(LuaError, "Lua thread not found");
		}
		else
		{
			PyErr_SetString(LuaError, "Lua function not found");
		}
		return NULL;
	}

	// check stack size
	if (lua_gettop(L) != stack_size)
	{
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

	return pReturn;
}

static PyObject* iter_LuaCoroutine(PyLua_LuaFunc* self)
{	
	if (!self->is_luathread)
	{
		PyErr_SetString(LuaError, "Not a lua thread");
		return NULL;
	}

	Py_INCREF(self);
	return self;
}


static PyObject* call_LuaFunc(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	if (self->is_luathread)
	{
		// raise error
		PyErr_SetString(LuaError, "Can not call lua thread type");
		return NULL;
	}

	if (kwargs)
	{
		// raise error
		PyErr_SetString(LuaError, "Lua function does not accept keyword arguments");
		return NULL;
	}

	lua_State* L = (lua_State*)self->lStack_prt;

	// make sure size of stact is the same at exit
	int stack_size = lua_gettop(L);

	// ensure space for all operations
	Py_ssize_t arg_len = PyTuple_Size(args);
	lua_checkstack(L, arg_len + 5);


	// error message if function is not found
	int found_func = 0;
	PyObject* pReturn = NULL;

	// get list of variables from lua
	lua_getglobal(L, "_G");

	lua_pushnil(L);

	// iterate over them to find the function
	while (lua_next(L, -2))
	{

		if (lua_topointer(L, -1) == self->lFunc_prt)
		{
			// found the function
			found_func = 1;

			// to find number of return values
			int current_stack = lua_gettop(L);

			// copy function
			lua_pushvalue(L, -1);

			for (int i = 0; i < arg_len; i++)
			{
				PyLua_PythonToLua(L, PyTuple_GetItem(args, i));
			}

			// call the function
			if (lua_pcall(L, arg_len, LUA_MULTRET, 0) != LUA_OK)
			{
				//return luaL_error(L, "Error: While calling the lua function.");

				PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(L, -1));
				return NULL;
			}

			int return_len = lua_gettop(L) - current_stack;

			pReturn = create_return(L, return_len);
			lua_pop(L, return_len);

		}

		lua_pop(L, 1);
	}

	lua_pop(L, 1); // remove _G

	if (!found_func)
	{
		//return luaL_error(L, "Error: Lua function not found");

		PyErr_SetString(LuaError, "Lua function not found");
		return NULL;
	}


	// check stack size
	if (lua_gettop(L) != stack_size)
	{
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

	return pReturn;

}

static PyObject* get_LuaFunc_Wrapper(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	if (kwargs)
	{
		// raise error
		PyErr_SetString(LuaError, "Lua function does not accept keyword arguments");
		return NULL;
	}

	uintptr_t a;
	uintptr_t b;
	int is_luathread;

	if (!PyArg_ParseTuple(args, "KKi", &a, &b, &is_luathread))
	{
		PyErr_SetString(LuaError, "Got wrong arguments to get_LuaFunc_Wrapper");
		return NULL;
	}

	self->lStack_prt = (void*)a;
	self->lFunc_prt = (void*)b;
	self->is_luathread = is_luathread;
	self->thread_terminated = 0;

	return 0;
}


static PyTypeObject pLuaFunc_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "pylua.lua_function_wrapper",
	.tp_doc = "pylua.lua_function_wrapper",
	.tp_basicsize = sizeof(PyLua_LuaFunc),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = get_LuaFunc_Wrapper,
	.tp_call = call_LuaFunc,
	.tp_iter = iter_LuaCoroutine,
	.tp_iternext = next_LuaCoroutine
};


static struct PyModuleDef LUA_module = {
	PyModuleDef_HEAD_INIT,
	"pylua",
	"pylua doc",
	-1,
};

PyMODINIT_FUNC PyInit_pylua(void)
{
	PyObject* m;

	if (PyType_Ready(&pLuaFunc_Type) < 0)
	{
		return NULL;
	}

	m = PyModule_Create(&LUA_module);
	if (m == NULL)
	{
		return NULL;
	}


	Py_INCREF(&pLuaFunc_Type);
	if (PyModule_AddObject(m, "lua_function_wrapper", (PyObject*)&pLuaFunc_Type) < 0) {
		Py_DECREF(&pLuaFunc_Type);
		Py_DECREF(m);
		return NULL;
	}

	LuaError = PyErr_NewException("pylua.LuaError", NULL, NULL);
	if (PyModule_AddObject(m, "error", LuaError) < 0) {
		Py_XDECREF(LuaError);
		Py_CLEAR(LuaError);
		Py_DECREF(m);
		return NULL;
	}

	return m;
}
