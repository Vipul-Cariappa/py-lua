#include "pylua.h"
#include "frameobject.h"

#include <wchar.h>
#include <string.h>
#include <stdlib.h>

#define TRACEBACK_STR_LEN 256
#define EXCEPTION_STR_LEN 256
#define LUA_MEMORY_ERROR(L) return luaL_error((L), "Memory Error")

// py_lua.c
PyMODINIT_FUNC PyInit_pylua(void);


int PyLua_PyLoadedModuleCount = 0;
extern PyObject* pPylua_Module;

typedef struct PyLua_PyModule {
	PyObject* module;
	int number;
	int loaded;
} PyLua_PyModule;


static int raise_error(lua_State* L, const char* msg)
{
	if (!PyErr_Occurred())
	{
		return luaL_error(L, msg);
	}

	size_t err_max = 1024;

	char* err_msg = malloc(err_max);
	if (!err_msg)
	{
		LUA_MEMORY_ERROR(L);
	}

	if (snprintf(err_msg, err_max, "%s\n\nPython Traceback:\n", msg) < 0)
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
			
			if (snprintf(err_msg, err_max, "%s", traceback_msg) < 0)
			{
				err_max *= 4;
				char* tmp = realloc(err_msg, err_max);

				if (tmp)
				{
					err_msg = tmp;
				}
				else
				{
					LUA_MEMORY_ERROR(L);
				}

				if (snprintf(err_msg, err_max, "%s", traceback_msg) < 0)
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
		err_max *= 2;
		char* tmp = realloc(err_msg, err_max);

		if (tmp)
		{
			err_msg = tmp;
		}
		else
		{
			LUA_MEMORY_ERROR(L);
		}

		if (!err_msg)
		{
			LUA_MEMORY_ERROR(L);
		}

		if (snprintf(err_msg, err_max, "%s", err_name) < 0)
		{
			LUA_MEMORY_ERROR(L);
		}
	}

	lua_pushstring(L, err_msg);

	Py_XDECREF(pExcType);
	Py_XDECREF(pExcValue);
	Py_XDECREF(pExcTraceback);

	Py_XDECREF(str_exc_value);
	Py_XDECREF(pExcValueStr);

	free(err_msg);
	free(err_name);
	free(new_strExcValue);

	return lua_error(L);
}


int call_PyFunc(lua_State* L)
{
	int args_count = lua_gettop(L);
	PyLua_PyFunc* py_callable = (PyLua_PyFunc*)lua_touserdata(L, lua_upvalueindex(1));

	if (!py_callable->function)
	{
		// raise error
		luaL_error(L, "Error: Python function not found");
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
		else if (PyErr_Occurred())
		{
			return raise_error(L, "Error: While calling python function");
		}
		else
		{
			// raise error which even I don't know
		}

	}

	LUA_MEMORY_ERROR(L);
}


int iter_PyGenerator(lua_State* L, ...)
// write big comment explaing the reason behind ...
{
	PyLua_PyIterator* py_iter = (PyLua_PyIterator*)lua_touserdata(L, lua_upvalueindex(1));

	PyObject* pItem = PyIter_Next(py_iter->iterator);
	if (pItem)
	{
		PyLua_PythonToLua(L, pItem);
		Py_DECREF(pItem);
		return lua_yieldk(L, 1, 0, iter_PyGenerator);
	}

	Py_DECREF(py_iter->iterator);

	if (PyErr_Occurred())
	{
		return raise_error(L, "Error: Occurred when iterating Python Object");
	}

	return luaL_error(L, "Error: Stop Iteration");
}


static int add_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyNumber_Add(py_obj->object, other);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}
	
	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int sub_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyNumber_Subtract(py_obj->object, other);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int mul_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyNumber_Multiply(py_obj->object, other);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int div_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyNumber_TrueDivide(py_obj->object, other);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int floordiv_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyNumber_FloorDivide(py_obj->object, other);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int pow_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyNumber_Power(py_obj->object, other, Py_None);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int mod_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyNumber_Remainder(py_obj->object, other);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int lshift_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyNumber_Lshift(py_obj->object, other);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int rshift_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyNumber_Rshift(py_obj->object, other);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int band_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyNumber_And(py_obj->object, other);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int bor_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyNumber_Or(py_obj->object, other);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int bxor_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyNumber_Xor(py_obj->object, other);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int eq_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyObject_RichCompare(py_obj->object, other, 2);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int lt_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyObject_RichCompare(py_obj->object, other, 0);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int le_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyObject_RichCompare(py_obj->object, other, 1);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int len_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* pReturn = PyObject_Size(py_obj->object);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int neg_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* pReturn = PyNumber_Negative(py_obj->object);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}

static int bnot_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	PyObject* pReturn = PyNumber_Invert(py_obj->object);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}


static int get_pythonobj_wrapper(lua_State* L)
{
	lua_getfield(L, 1, "__python");
	PyLua_PyObject* py_obj = (PyLua_PyObject*)lua_touserdata(L, -1);
	lua_pop(L, 1); // remove userdata

	// attribute to string
	const char* attr = lua_tostring(L, 2);

	PyObject* other = PyLua_LuaToPython(L, 2);
	PyObject* pReturn = PyObject_GetAttrString(py_obj->object, attr);
	if (pReturn)
	{
		return PyLua_PythonToLua(L, pReturn);
	}

	return raise_error(L, "Error: Occurred when getting Python Object attribute");
}


static int PyLua_PyLoadModule(lua_State* L)
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
		if (!pPylua_Module)
		{
			return raise_error(L, "Error: could not import module 'pylua'");
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

	return raise_error(L, "Error: Occurred when getting Python Object");
}


static const struct luaL_Reg PY_lib[] = {
	{"PyLoad", PyLua_PyLoadModule},
	{"PyUnLoad", PyLua_PyUnloadModule},
	{NULL, NULL}
};

static const struct luaL_Reg PY_Call_Wrapper[] = {
	{"__add", add_pythonobj_wrapper},
	{"__sub", sub_pythonobj_wrapper},
	{"__mul", mul_pythonobj_wrapper},
	{"__div", div_pythonobj_wrapper},
	{"__idiv", floordiv_pythonobj_wrapper},
	{"__mod", mod_pythonobj_wrapper},
	{"__pow", pow_pythonobj_wrapper},
	{"__band", band_pythonobj_wrapper},
	{"__bor", bor_pythonobj_wrapper},
	{"__bxor", bxor_pythonobj_wrapper},
	{"__shl", lshift_pythonobj_wrapper},
	{"__shr", rshift_pythonobj_wrapper},
	{"__eq", eq_pythonobj_wrapper},
	{"__lt", lt_pythonobj_wrapper},
	{"__le", le_pythonobj_wrapper},
	{"__len", len_pythonobj_wrapper},
	{"__unm", neg_pythonobj_wrapper},
	{"__bnot", bnot_pythonobj_wrapper},
	{"__index", get_pythonobj_wrapper},
	{NULL, NULL}
};


#if defined(_WIN32)
__declspec(dllexport)
#endif
int luaopen_pylua(lua_State* L) {
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
	lua_setglobal(L, "PythonClassWrapper");

	return 1;
}
