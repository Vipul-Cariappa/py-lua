#include "lua_py.h"
#include <dlfcn.h>
#include <wchar.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define PYLIB_PATH                                                             \
  "/usr/lib/x86_64-linux-gnu/libpython" STR(PY_MAJOR_VERSION) "." STR(         \
      PY_MINOR_VERSION) ".so"

int PyLua_PyLoadedModuleCount = 0;


#if defined(_WIN32)
__declspec(dllexport)
#endif
int luaopen_pylua(lua_State* L)
{
	SAVE_STACK_SIZE(L);

	if (DrivingLang == -1)
	{
		DrivingLang = 0;

		// initialise python interpreter
		if (PyImport_AppendInittab("pylua", PyInit_pylua) == -1) {
			return luaL_error(L, "Error: could not extend in-built modules table");
		}

#if defined (__linux__)
	dlopen(PYLIB_PATH, RTLD_LAZY | RTLD_GLOBAL);	// TODO: handle errors
#endif

		Py_Initialize();

		PySys_SetArgv(0, NULL);

		pPylua_Module = PyImport_ImportModule("pylua");
		if (!pPylua_Module)
		{
			return raise_error(L, "Error: could not import module 'pylua'");
		}
	}

	lua_pushvalue(L, LUA_REGISTRYINDEX);
	cL = lua_newthread(L);
	lua_setfield(L, -2, "pylua thread");
	lua_pop(L, 1);

	// python module
	luaL_newlib(L, PY_lib);

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
	lua_setmetatable(L, -2);

	lua_setglobal(L, "Python");

	luaL_newlib(L, PY_Call_Wrapper);
	lua_pushboolean(L, 1);
	lua_setfield(L, -2, "__python");
	lua_setglobal(L, "PythonClassWrapper");

	CHECK_STACK_SIZE(L, 0);

	return 0;
}


static const struct luaL_Reg PY_lib[3] = {
	{"PyLoad", &PyLua_PyLoadModule},
	{"PyUnLoad", &PyLua_PyUnloadModule},
	{NULL, NULL}
};


static const struct luaL_Reg PY_Call_Wrapper[24] = {
	{"__add", &add_PyObj},
	{"__sub", &sub_PyObj},
	{"__mul", &mul_PyObj},
	{"__div", &div_PyObj},
	{"__idiv", &floordiv_PyObj},
	{"__mod", &mod_PyObj},
	{"__pow", &pow_PyObj},
	{"__band", &band_PyObj},
	{"__bor", &bor_PyObj},
	{"__bxor", &bxor_PyObj},
	{"__shl", &lshift_PyObj},
	{"__shr", &rshift_PyObj},
	{"__eq", &eq_PyObj},
	{"__lt", &lt_PyObj},
	{"__le", &le_PyObj},
	{"__len", &len_PyObj},
	{"__unm", &neg_PyObj},
	{"__bnot", &bnot_PyObj},
	{"__index", &get_PyObj},
	{"__newindex", &set_PyObj},
	{"__tostring", &str_PyObj},
	{"__call", &call_PyObj},
	{"__gc", &gc_PyObj},
	{NULL, NULL}
};


// Python Wrapper functions

int call_PyFunc(lua_State* L)
{
	int args_count = lua_gettop(L);

	PyLua_PyFunc* py_callable = (PyLua_PyFunc*)lua_touserdata(L, lua_upvalueindex(1));

	if (!py_callable->function)
	{
		// raise error
		luaL_error(L, LERR_FUNC_NOT_FOUND);
	}

	PyObject* pArgs = PyTuple_New(args_count);
	PyObject* pItem;

	if (pArgs)
	{
		for (int i = 0, j = 1; i < args_count; i++, j++)
		{
			pItem = PyLua_LuaToPython(L, j);
			if (!pItem)
			{
				return raise_error(L, LERR_CONVERT_LUA_TO_PY);
			}
			PyTuple_SetItem(pArgs, i, pItem);
		}

		PyObject* pResult = PyObject_CallObject(py_callable->function, pArgs);
		Py_DECREF(pArgs);
		if (pResult)
		{
			if (PyLua_PythonToLua(L, pResult) < 0)
			{
				Py_DECREF(pResult);
				return raise_error(L, LERR_CONVERT_PY_TO_LUA);
			}
			Py_DECREF(pResult);

			return 1;
		}

		return raise_error(L, ERR_CALL_PY);
	}

	LUA_MEMORY_ERROR(L);
}

int iter_PyGenerator(lua_State* L, int _a1, long _a2)
{
	PyLua_PyIterator* py_iter = (PyLua_PyIterator*)lua_touserdata(L, lua_upvalueindex(1));

	PyObject* pItem = PyIter_Next(py_iter->iterator);
	if (pItem)
	{
		if (PyLua_PythonToLua(L, pItem) < 0)
		{
			Py_DECREF(pItem);
			return raise_error(L, LERR_CONVERT_PY_TO_LUA);
		}
		Py_DECREF(pItem);
		return lua_yieldk(L, 1, 0, iter_PyGenerator);
	}

	Py_DECREF(py_iter->iterator);

	if (PyErr_Occurred())
	{
		return raise_error(L, ERR_CALL_PY);
	}

	return luaL_error(L, LERR_STOP_ITER);
}

static int get_PyObj(lua_State* L)
{	
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, 1);

	// attribute to string
	const char* attr = lua_tostring(L, 2);

	PyObject* pReturn = PyObject_GetAttrString(py_obj->object, attr);
	if (pReturn)
	{
		if (PyLua_PythonToLua(L, pReturn) < 0)
		{
			Py_DECREF(pReturn);
			return raise_error(L, LERR_CONVERT_PY_TO_LUA);
		}
		Py_DECREF(pReturn);
		return 1;
	}
	else
	{
		if (PyMapping_Check(py_obj->object))
		{
			pReturn = PyMapping_GetItemString(py_obj->object, attr);
			if (pReturn)
			{
				if (PyLua_PythonToLua(L, pReturn) < 0)
				{
					Py_DECREF(pReturn);
					return raise_error(L, LERR_CONVERT_PY_TO_LUA);
				}
				Py_DECREF(pReturn);
				return 1;
			}
		}
		else if (PySequence_Check(py_obj->object))
		{
			long long i = atoll(attr);
			pReturn = PySequence_GetItem(py_obj->object, i);
			if (pReturn)
			{
				if (PyLua_PythonToLua(L, pReturn) < 0)
				{
					Py_DECREF(pReturn);
					return raise_error(L, LERR_CONVERT_PY_TO_LUA);
				}
				Py_DECREF(pReturn);
				return 1;
			}
		}
	}

	return raise_error(L, LERR_GET_ATTR);
}

static int set_PyObj(lua_State* L)
{
	// TODO: implement
	return 0;
}

static int str_PyObj(lua_State* L)
{
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, 1);

	PyObject* pReturn = PyObject_Str(py_obj->object);
	if (pReturn)
	{
		PyObject* encodedString = PyUnicode_AsEncodedString(pReturn, "UTF-8", "strict");
		if (encodedString)
		{
			const char* result = PyBytes_AsString(encodedString);
			if (result) {
				lua_pushstring(L, result);
			}
			Py_DECREF(encodedString);
		}
		else
		{
			return raise_error(L, MEMROY_ERR);
		}
		
		Py_DECREF(pReturn);
		return 1;
	}

	return raise_error(L, ERR_CALL_PY);
}

static int call_PyObj(lua_State* L)
{
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, 1);

	int args_count = lua_gettop(L) - 1;

	PyObject* pArgs = PyTuple_New(args_count);

	if (!pArgs)
	{
		return raise_error(L, MEMROY_ERR);
	}

	for (int i = 0, j = 2; i < args_count; i++, j++)
	{
		PyObject* pItem = PyLua_LuaToPython(L, j);
		PyTuple_SetItem(pArgs, i, pItem);
	}

	PyObject* pReturn = PyObject_CallObject(py_obj->object, pArgs);
	Py_DECREF(pArgs);

	if (pReturn)
	{
		if (PyLua_PythonToLua(L, pReturn) < 0)
		{
			Py_DECREF(pReturn);
			return raise_error(L, LERR_CONVERT_PY_TO_LUA);
		}
		Py_DECREF(pReturn);
		return 1;
	}

	return raise_error(L, ERR_CALL_PY);
}

static int gc_PyObj(lua_State* L)
{
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, 1);
	Py_DECREF(py_obj->object);

	return 0;
}

static int add_PyObj(lua_State* L)
{
	return binary_base_pyobj_wrapper(L, PyNumber_Add);
}

static int sub_PyObj(lua_State* L)
{
	return binary_base_pyobj_wrapper(L, PyNumber_Subtract);
}

static int mul_PyObj(lua_State* L)
{
	return binary_base_pyobj_wrapper(L, PyNumber_Multiply);
}

static int div_PyObj(lua_State* L)
{
	return binary_base_pyobj_wrapper(L, PyNumber_TrueDivide);
}

static int floordiv_PyObj(lua_State* L)
{
	return binary_base_pyobj_wrapper(L, PyNumber_FloorDivide);
}

static int pow_PyObj(lua_State* L)
{
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, 1);

	PyObject* other = PyLua_LuaToPython(L, 2);
	if (!other)
	{
		return raise_error(L, LERR_CONVERT_LUA_TO_PY);
	}

	PyObject* pReturn = PyNumber_Power(py_obj->object, other, Py_None);

	Py_DECREF(other);

	if (pReturn)
	{
		if (PyLua_PythonToLua(L, pReturn) < 0)
		{
			Py_DECREF(pReturn);
			return raise_error(L, LERR_CONVERT_PY_TO_LUA);
		}
		Py_DECREF(pReturn);
		return 1;
	}

	return raise_error(L, ERR_CALL_PY);
}

static int mod_PyObj(lua_State* L)
{
	return binary_base_pyobj_wrapper(L, PyNumber_Remainder);
}

static int lshift_PyObj(lua_State* L)
{
	return binary_base_pyobj_wrapper(L, PyNumber_Lshift);
}

static int rshift_PyObj(lua_State* L)
{
	return binary_base_pyobj_wrapper(L, PyNumber_Rshift);
}

static int band_PyObj(lua_State* L)
{
	return binary_base_pyobj_wrapper(L, PyNumber_And);
}

static int bor_PyObj(lua_State* L)
{
	return binary_base_pyobj_wrapper(L, PyNumber_Or);
}

static int bxor_PyObj(lua_State* L)
{
	return binary_base_pyobj_wrapper(L, PyNumber_Xor);
}

static int eq_PyObj(lua_State* L)
{
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, 1);

	PyObject* other = PyLua_LuaToPython(L, 2);
	if (!other)
	{
		return raise_error(L, LERR_CONVERT_LUA_TO_PY);
	}

	PyObject* pReturn = PyObject_RichCompare(py_obj->object, other, 2);

	Py_DECREF(other);

	if (pReturn)
	{
		if (PyLua_PythonToLua(L, pReturn) < 0)
		{
			Py_DECREF(pReturn);
			return raise_error(L, LERR_CONVERT_PY_TO_LUA);
		}
		Py_DECREF(pReturn);
		return 1;
	}

	return raise_error(L, ERR_CALL_PY);
}

static int lt_PyObj(lua_State* L)
{
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, 1);

	PyObject* other = PyLua_LuaToPython(L, 2);
	if (!other)
	{
		return raise_error(L, LERR_CONVERT_LUA_TO_PY);
	}

	PyObject* pReturn = PyObject_RichCompare(py_obj->object, other, 0);

	Py_DECREF(other);

	if (pReturn)
	{
		if (PyLua_PythonToLua(L, pReturn) < 0)
		{
			Py_DECREF(pReturn);
			return raise_error(L, LERR_CONVERT_PY_TO_LUA);
		}
		Py_DECREF(pReturn);
		return 1;
	}

	return raise_error(L, ERR_CALL_PY);
}

static int le_PyObj(lua_State* L)
{
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, 1);

	PyObject* other = PyLua_LuaToPython(L, 2);
	if (!other)
	{
		return raise_error(L, LERR_CONVERT_LUA_TO_PY);
	}

	PyObject* pReturn = PyObject_RichCompare(py_obj->object, other, 1);

	Py_DECREF(other);

	if (pReturn)
	{
		if (PyLua_PythonToLua(L, pReturn) < 0)
		{
			Py_DECREF(pReturn);
			return raise_error(L, LERR_CONVERT_PY_TO_LUA);
		}
		Py_DECREF(pReturn);
		return 1;
	}

	return raise_error(L, ERR_CALL_PY);
}

static int len_PyObj(lua_State* L)
{
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, 1);

	Py_ssize_t len = PyObject_Size(py_obj->object);
	if (len >= 0)
	{
		lua_pushinteger(L, len);
		return 1;
	}

	return raise_error(L, ERR_CALL_PY);
}

static int neg_PyObj(lua_State* L)
{
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, 1);

	PyObject* pReturn = PyNumber_Negative(py_obj->object);
	if (pReturn)
	{
		if (PyLua_PythonToLua(L, pReturn) < 0)
		{
			Py_DECREF(pReturn);
			return raise_error(L, LERR_CONVERT_PY_TO_LUA);
		}
		Py_DECREF(pReturn);
		return 1;
	}

	return raise_error(L, ERR_CALL_PY);
}

static int bnot_PyObj(lua_State* L)
{
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, 1);

	PyObject* pReturn = PyNumber_Invert(py_obj->object);
	if (pReturn)
	{
		if (PyLua_PythonToLua(L, pReturn) < 0)
		{
			Py_DECREF(pReturn);
			return raise_error(L, LERR_CONVERT_PY_TO_LUA);
		}
		Py_DECREF(pReturn);
		return 1;
	}

	return raise_error(L, ERR_CALL_PY);
}


// Lua Module Functions

static int PyLua_PyLoadModule(lua_State* L)
{
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

	return raise_error(L, "Error: Could Not Import the Python Module");
}

static int PyLua_PyUnloadModule(lua_State* L)
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

static int PyLua_PySet(lua_State* L)
{
	return luaL_error(L, "Error: Cannot Assign Values to Python.Module Objects");
}

static int PyLua_PyGet(lua_State* L)
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
		return x;
	}

	return raise_error(L, LERR_GET_ATTR);
}
