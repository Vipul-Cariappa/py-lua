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
	void* a;
} PyLua_LuaFunc;


PyObject* call_LuaFunc(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	return exec_LuaFunc(self->a, args, kwargs);
}

PyObject* get_LuaFunc_Wrapper(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	uintptr_t a;
	if (!PyArg_ParseTuple(args, "K", &a))
	{
		return NULL;
	}

	self->a = (void*)a;

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
