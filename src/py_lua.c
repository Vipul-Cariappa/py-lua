#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdint.h>

// convert.c
int PyLua_PythonToLua(lua_State* L, PyObject* pItem);
PyObject* PyLua_LuaToPython(lua_State* L, int index);

// callable.c
PyObject* exec_LuaFunc(void* data, PyObject* args, PyObject* kwargs);


typedef struct PyLua_LuaFunc {
	PyObject_HEAD
	void* lStack_prt;
	void* lFunc_prt;
} PyLua_LuaFunc;


PyObject* call_LuaFunc(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	lua_State* L = (lua_State*)self->lStack_prt;

	// make sure size of stact is the same at exit
	int stack_size = lua_gettop(L);

	// ensure space for all operations
	Py_ssize_t arg_len = PyTuple_Size(args);
	lua_checkstack(L, 5 + arg_len);

	
	// error message if function is not found
	int found_func = 0;
	PyObject* return_value = NULL;

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
			if (lua_pcall(L, arg_len, LUA_MULTRET, 0) == LUA_ERRRUN)
			{
				printf("Error Msg: %s\n", lua_tostring(L, -1));
				return luaL_error(L, "Error: While calling the lua function.");
			}

			int return_len = lua_gettop(L) - current_stack;

			if (return_len == 0)
			{
				Py_INCREF(Py_None);
				return_value = Py_None;
			}
			else if (return_len == 1)
			{
				return_value = PyLua_LuaToPython(L, -1);
				lua_pop(L, 1);
			}
			else
			{
				// to be implemented
				Py_INCREF(Py_False);
				return_value = Py_False;

				lua_pop(L, lua_gettop(L) - 3);
			}

		}
		
		lua_pop(L, 1);
	}
	
	lua_pop(L, 1);

	if (!found_func)
	{
		return luaL_error(L, "Error: Lua function not found");
	}


	// check stack size
	if (lua_gettop(L) != stack_size)
	{
		return luaL_error(L, "Error: Stack size not same");
	}

	return return_value;
	
}

PyObject* get_LuaFunc_Wrapper(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	uintptr_t a;
	uintptr_t b;

	if (!PyArg_ParseTuple(args, "KK", &a, &b))
	{
		return NULL;
	}

	self->lStack_prt = (void*)a;
	self->lFunc_prt = (void*)b;

	return 0;
}


static PyTypeObject MyObject_Type = {
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

//static PyMethodDef LUA_lib[] = {
//	{NULL, NULL, 0, NULL}
//};

static struct PyModuleDef LUA_module = {
	PyModuleDef_HEAD_INIT,
	"pylua",
	"pylua doc",
	-1,
};

PyMODINIT_FUNC PyInit_pylua(void)
{
	PyObject* m;

	if (PyType_Ready(&MyObject_Type) < 0)
	{
		return NULL;
	}

	m = PyModule_Create(&LUA_module);
	if (m == NULL)
	{
		return NULL;
	}

	Py_INCREF(&MyObject_Type);
	if (PyModule_AddObject(m, "lua_function_wrapper", (PyObject*)&MyObject_Type) < 0) {
		Py_DECREF(&MyObject_Type);
		Py_DECREF(m);
		return NULL;
	}

	return m;
}
