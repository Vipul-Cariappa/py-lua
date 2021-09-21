#include "pylua.h"


typedef struct PyLua_LuaFunc {
	PyObject_HEAD
	void* lStack_prt;
	void* lFunc_prt;
} PyLua_LuaFunc;


static PyObject* LuaError;


PyObject* call_LuaFunc(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	if (kwargs)
	{
		// raise error
	}

	lua_State* L = (lua_State*)self->lStack_prt;

	// make sure size of stact is the same at exit
	int stack_size = lua_gettop(L);

	// ensure space for all operations
	Py_ssize_t arg_len = PyTuple_Size(args);
	lua_checkstack(L, 5 + arg_len);


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

				PyErr_SetString(LuaError, lua_tostring(L, -1));
				return NULL;
			}

			int return_len = lua_gettop(L) - current_stack;

			if (return_len == 0)
			{
				Py_INCREF(Py_None);
				pReturn = Py_None;
			}
			else if (return_len == 1)
			{
				pReturn = PyLua_LuaToPython(L, -1);
				lua_pop(L, 1);
			}
			else
			{
				// to be implemented
				Py_INCREF(Py_False);
				pReturn = Py_False;

				lua_pop(L, return_len);
			}

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
		return luaL_error(L, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

	return pReturn;

}

PyObject* get_LuaFunc_Wrapper(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	if (kwargs)
	{
		// raise error
	}

	uintptr_t a;
	uintptr_t b;

	if (!PyArg_ParseTuple(args, "KK", &a, &b))
	{
		return NULL;
	}
	else
	{
		// raise error
	}

	self->lStack_prt = (void*)a;
	self->lFunc_prt = (void*)b;

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
