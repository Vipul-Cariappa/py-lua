#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <wchar.h>

// convert.c
int PyLua_PythonToLua(lua_State* L, PyObject* pItem);

// callable.c
PyMODINIT_FUNC PyInit_pylua(void);

int PyLua_PyLoadedModuleCount = 0;
extern PyObject* PyLua_pylua_module;


typedef struct PyLua_PyModule {
	PyObject* module;
	int number;
	int loaded;
} PyLua_PyModule;


int PyLua_PyLoadModule(lua_State* L)
{
	if (!Py_IsInitialized())
	{
		if (PyImport_AppendInittab("pylua", PyInit_pylua) == -1) {
			return luaL_error(L, "Error: could not extend in-built modules table");
		}

		Py_Initialize();

		wchar_t* path = Py_GetPath();

		#if defined(_WIN32)
		wchar_t new_path[800];
		if (!new_path)
		{
			exit(-1);
		}
		new_path[0] = L'.';
		new_path[1] = L';';
		new_path[2] = L'\0';
		#else
		wchar_t new_path[800];
		if (!new_path)
		{
			exit(-1);
		}
		new_path[0] = L'.';
		new_path[1] = L':';
		new_path[2] = L'\0';
		#endif

		wcsncat(new_path, path, wcslen(path));

		PySys_SetPath(new_path);

		PyLua_pylua_module = PyImport_ImportModule("pylua");
		if (!PyLua_pylua_module) {
			return luaL_error(L, "Error: could not import module 'pylua'");
		}

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
		py_module->loaded = 1;

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
	luaL_argcheck(L, py_module != NULL, 1, "Error: 'Python.Module' expected");

	if (py_module->module && py_module->loaded)
	{
		PyLua_PyLoadedModuleCount--;
		py_module->loaded = 0;

		Py_DECREF(py_module->module);
	}
	else
	{
		return luaL_error(L, "Error: Specified Python.Module has already been unloaded.");
	}


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

	if (!py_module->loaded)
	{
		return luaL_error(L, "Error: Specified Python.Module has been unloaded. \nAttempt to index unloaded Python.Module");
	}

	// getting python object of the given name
	const char* name = luaL_checkstring(L, 2);
	PyObject* pItem = PyObject_GetAttrString(py_module->module, name);

	// converting to lua object
	if (pItem)
	{
		int x = PyLua_PythonToLua(L, pItem);
		if (x != -1)
		{
			return x;
		}

		Py_DECREF(pItem);
		return luaL_error(L, "Error: Problem Occured while converting python object to lua variable");
	}

	return luaL_error(L, "Error: Some Error Occurred when getting Python Object");
}
