#include "pylua.h"

#include <wchar.h>

// py_lua.c
PyMODINIT_FUNC PyInit_pylua(void);

int PyLua_PyLoadedModuleCount = 0;
extern PyObject* pPylua_Module;


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

		pPylua_Module = PyImport_ImportModule("pylua");
		if (!pPylua_Module) {
			if (PyErr_Occurred())
			{
				PyErr_Print();
			}
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

	if (PyErr_Occurred())
	{
		PyErr_Print();
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


int call_PyFunc(lua_State* L)
{
	int args_count = lua_gettop(L);
	PyLua_PyCallable* py_callable = (PyLua_PyCallable*)lua_touserdata(L, lua_upvalueindex(1));

	if (!py_callable->function)
	{
		// raise error
		luaL_error(L, "Error: Python function out of bound");
	}

	PyObject* pArgs = PyTuple_New(args_count);
	PyObject* pItem;

	if (pArgs)
	{
		for (int i = 0, j = 1; i < args_count; i++, j++)
		{
			pItem = PyLua_LuaToPython(L, j);
			PyTuple_SetItem(pArgs, i, pItem);
		}

		PyObject* pResult = PyObject_CallObject(py_callable->function, pArgs);
		Py_DECREF(pArgs);
		if (pResult)
		{
			PyLua_PythonToLua(L, pResult);
			Py_DECREF(pResult);

			return 1;
		}
		else
		{
			PyErr_Print();
			return luaL_error(L, "Error: While executing function\n");
		}

	}

	return luaL_error(L, "Error: Memory Error\n");
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
		return luaL_error(L, "Error: Occured while converting python object to lua variable");
	}

	if (PyErr_Occurred())
	{
		PyErr_Print();
	}
	return luaL_error(L, "Error: Occurred when getting Python Object");
}


int init_iter(lua_State* L, ...)
{
	PyLua_PyIterator* py_iter = (PyLua_PyIterator*)lua_touserdata(L, lua_upvalueindex(1));

	PyObject* pItem = PyIter_Next(py_iter->iterator);
	if (pItem)
	{
		PyLua_PythonToLua(L, pItem);
		Py_DECREF(pItem);
		return lua_yieldk(L, 1, NULL, init_iter);
	}

	PyErr_Print();
	return luaL_error(L, "Error: Iteration\n");
}


static const struct luaL_Reg PY_lib[] = {
	{"PyLoad", PyLua_PyLoadModule},
	{"PyUnLoad", PyLua_PyUnloadModule},
	{NULL, NULL}
};


#if defined(_WIN32)
__declspec(dllexport)
#endif
int luaopen_pylua(lua_State* L) {
	// python module
	luaL_newmetatable(L, "Python.Module");
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, PyLua_PyUnloadModule);
	lua_settable(L, -3);
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, PyLua_PyGet);
	lua_settable(L, -3);
	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, PyLua_PySet);
	lua_settable(L, -3);
	luaL_setfuncs(L, PY_lib, 0);
	lua_setglobal(L, "Python");

	return 1;
}
