#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// convert.c
int PyLua_PythonToLua(lua_State* L, PyObject* pItem, PyObject* pModule);

// callable.c
PyMODINIT_FUNC PyInit_pylua(void);

int PyLua_PyLoadedModuleCount = 0;
PyObject* PyLua_pylua_module;


typedef struct PyLua_PyModule {
	PyObject* module;
	int number;
} PyLua_PyModule;


int PyLua_PyLoadModule(lua_State* L)
{
	if (!Py_IsInitialized())
	{
		if (PyImport_AppendInittab("pylua", PyInit_pylua) == -1) {
			return luaL_error(L, "Error: could not extend in-built modules table");
		}

		Py_Initialize();

		PyLua_pylua_module = PyImport_ImportModule("pylua");
		if (!PyLua_pylua_module) {
			return luaL_error(L, "Error: could not import module 'pylua'");
		}

		PySys_SetPath(L".");
	}

	const char* module_name = luaL_checkstring(L, 1);
	PyObject* pModule = PyImport_ImportModule(module_name);

	if (pModule)
	{
		PyLua_PyLoadedModuleCount++;

		// creating lua userdatum from python module
		size_t nbytes = sizeof(PyLua_PyModule);
		PyLua_PyModule* py_module = (PyLua_PyModule*)lua_newuserdata(L, nbytes);
		py_module->module = pModule;
		py_module->number = PyLua_PyLoadedModuleCount;

		// registrying userdatum to lua
		luaL_getmetatable(L, "Python.Module");
		lua_setmetatable(L, -2);

		return 1;
	}

	return luaL_error(L, "Error: Could Not Import the Python Module");
}


int PyLua_PyUnloadModule(lua_State* L)
{
	PyLua_PyModule* py_module = (PyLua_PyModule*)luaL_checkudata(L, 1, "Python.Module");
	luaL_argcheck(L, py_module != NULL, 1, "'Python.Module' expected");
	PyLua_PyLoadedModuleCount--;
	Py_DECREF(py_module->module);

	if ((PyLua_PyLoadedModuleCount == 0) && (Py_IsInitialized()))
	{
		Py_Finalize();
	}

	return 0;
}


int PyLua_PySet(lua_State* L)
{
	return luaL_error(L, "Error: Cannot Assign Values to Python.Module Objects");
}


int PyLua_PyGet(lua_State* L)
{
	PyLua_PyModule* py_module = (PyLua_PyModule*)luaL_checkudata(L, 1, "Python.Module");
	luaL_argcheck(L, py_module != NULL, 1, "'Python.Module' expected");

	// getting python object of the given name
	const char* name = luaL_checkstring(L, 2);
	PyObject* pItem = PyObject_GetAttrString(py_module->module, name);

	// converting to lua object
	if (pItem)
	{
		int x = PyLua_PythonToLua(L, pItem, py_module->module);
		if (x != -1)
		{
			return x;
		}

		Py_DECREF(pItem);
		return luaL_error(L, "Error: Problem Occured while converting python object to lua variable.");
	}

	return luaL_error(L, "Error: Some Error Occurred when getting Python Object");
}
