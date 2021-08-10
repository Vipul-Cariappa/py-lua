#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// callable.c
void get_pylua_func(lua_State* L, PyObject* pyfunc, PyObject* pymodule);

int PY_module_count = 0;

typedef struct PY_module {
	PyObject* module;
	int number;
} PY_module;

int PY_load(lua_State* L)
{
	if (!Py_IsInitialized())
	{
		Py_Initialize();
		PySys_SetPath(L".");
	}

	const char* module_name = luaL_checkstring(L, 1);
	PyObject* module = PyImport_ImportModule(module_name);
	if (module)
	{
		PY_module_count++;

		// creating lua userdatum from python module
		size_t nbytes = sizeof(PY_module);
		PY_module* luapy_object = (PY_module*)lua_newuserdata(L, nbytes);
		luapy_object->module = module;
		luapy_object->number = PY_module_count;

		// registrying userdatum to lua
		luaL_getmetatable(L, "Python.Module");
		lua_setmetatable(L, -2);

		return 1;
	}

	return luaL_error(L, "Some Internal Error Occurred");
}

int PY_unload(lua_State* L)
{
	PY_module* luapy_object = (PY_module*)luaL_checkudata(L, 1, "Python.Module");
	luaL_argcheck(L, luapy_object != NULL, 1, "'Python.Module' expected");
	PY_module_count--;
	Py_DECREF(luapy_object->module);

	if ((PY_module_count == 0) && (Py_IsInitialized()))
	{
		Py_Finalize();
	}
	return 0;
}


int set(lua_State* L)
{
	return luaL_error(L, "Cannot Assign Values to Python.Module Objects");
}

int get(lua_State* L)
{
	PY_module* luapy_object = (PY_module*)luaL_checkudata(L, 1, "Python.Module");
	luaL_argcheck(L, luapy_object != NULL, 1, "'Python.Module' expected");

	const char* attr_name = luaL_checkstring(L, 2);
	PyObject* pyitem = PyObject_GetAttrString(luapy_object->module, attr_name);

	if (pyitem)
	{
		if (PyFloat_Check(pyitem))
		{
			double result = PyFloat_AsDouble(pyitem);
			lua_pushnumber(L, result);
			Py_DECREF(pyitem);

			return 1;
		}
		else if (PyCallable_Check(pyitem))
		{
			get_pylua_func(L, pyitem, luapy_object->module);
			return 1;
		}
	}

	return luaL_error(L, "Some Error Occurred when getting Python Object");
}
