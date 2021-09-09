#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdint.h>

#define DEFAULT_BUFF_SIZE 1000

// convert.c
PyObject* PyLua_LuaToPython(lua_State* L, int index);
int PyLua_PythonToLua(lua_State* L, PyObject* pItem);

// bindings.c
//PyObject* get_pylua_module();
PyObject* PyLua_pylua_module;


typedef struct {
	char* buffer;
	size_t Ridx;
	size_t Widx;
	size_t size;
} lua_ObjectBuffer;


const char* lReader(lua_State* L, void* data, size_t* size)
{
	const char* rvalue = NULL;
	lua_ObjectBuffer* lo = (lua_ObjectBuffer*)data;

	size_t available = lo->Widx - lo->Ridx;
	*size = available;

	if (available != 0)
	{
		rvalue = lo->buffer + lo->Ridx;
	}


	lo->Ridx += available;
	return rvalue;
}

int lWriter(lua_State* L, const void* p, size_t sz, void* ud)
{
	lua_ObjectBuffer* lo = (lua_ObjectBuffer*)(ud);

	if (!(lo->buffer))
	{
		lo->buffer = malloc(DEFAULT_BUFF_SIZE);
		if (!(lo->buffer))
		{
			return -1;
		}
		lo->size = DEFAULT_BUFF_SIZE;
	}

	if (sz > lo->size - lo->Widx)
	{
		lo->size += sz + 1000;
		void* tmp = lo->buffer;
		lo->buffer = realloc(tmp, lo->size);
		if (!(lo->buffer))
		{
			return -1;
		}
	}

	memcpy((char*)lo->buffer + lo->Widx, p, sz);
	lo->Widx += sz;
	return 0;
}

PyObject* exec_LuaFunc(void* data, PyObject* args, PyObject* kwargs)
{
	lua_State* newL = luaL_newstate();
	luaL_openlibs(newL);
	lua_ObjectBuffer* lo = (lua_ObjectBuffer*)data;
	int err = lua_load(newL, lReader, lo, "transferFunction", NULL);
	int stack_size = lua_gettop(newL) - 1;

	if (err == 0)
	{
		Py_ssize_t args_count = PyTuple_Size(args);

		for (int i = 0; i < args_count; i++)
		{
			PyObject* pItem = PyTuple_GetItem(args, i);
			PyLua_PythonToLua(newL, pItem);
		}

		if (lua_pcall(newL, args_count, -1, 0) == LUA_OK)
		{
			printf("Called lua function\n");
			int return_size = lua_gettop(newL) - stack_size;

			if (return_size == 0)
			{
				Py_RETURN_NONE;
			}
			else if (return_size == 1)
			{
				return PyLua_LuaToPython(newL, -1);
			}
			else
			{
				PyObject* pReturn = PyTuple_New(return_size);

				for (int i = -1, j = 0; j < return_size; i--, j++)
				{
					PyTuple_SetItem(pReturn, j, PyLua_LuaToPython(newL, i));
				}

				return pReturn;
			}
			Py_RETURN_NONE;
		}
		else
		{
			printf("%s", lua_tostring(newL, -1));
			return luaL_error(newL, "Error: Execution of lua function!\n");
		}

	}
	else if (err == LUA_ERRMEM)
	{
		return luaL_error(newL, "Error: In lua function memory error 'exec_LuaFunc'\n");
	}
	else
	{
		return luaL_error(newL, "Error: In calling python function 'exec_LuaFunc'");
	}
}

PyObject* get_LuaFunc(lua_State* L, int index)
{
	lua_ObjectBuffer lo = { NULL, 0, 0, 0 };

	lua_pushvalue(L, index);

	if (lua_dump(L, lWriter, &lo, 0) == 0)
	{
		uintptr_t prtvalue = &lo;

		PyObject* func = PyObject_GetAttrString(PyLua_pylua_module, "lua_function_wrapper");

		PyObject* pArgs = Py_BuildValue("(K)", lo);

		PyObject* rvalue = PyObject_CallObject(func, pArgs);

		lua_pop(L, 1);
		return rvalue;
	}

	lua_pop(L, 1);
}


typedef struct PyLua_PyCallable {
	PyObject* function;
} PyLua_PyCallable;


int PyLua_PyCallFunc(lua_State* L)
{
	int args_count = lua_gettop(L);
	PyLua_PyCallable* luapy_callable = (PyLua_PyCallable*)lua_touserdata(L, lua_upvalueindex(1));

	PyObject* pArgs = PyTuple_New(args_count);
	PyObject* pItem;

	if (pArgs)
	{
		for (int i = 0, j = 1; i < args_count; i++, j++)
		{
			pItem = PyLua_LuaToPython(L, j);
			PyTuple_SetItem(pArgs, i, pItem);
		}

		PyObject* pResult = PyObject_CallObject(luapy_callable->function, pArgs);
		Py_DECREF(pArgs);
		if (pResult)
		{
			PyLua_PythonToLua(L, pResult);
			Py_DECREF(pResult);

			Py_DECREF(luapy_callable->function);
			return 1;
		}

		Py_DECREF(luapy_callable->function);
		return luaL_error(L, "Error: Some Internal Problem\n");
	}

	Py_DECREF(luapy_callable->function);
	return luaL_error(L, "Error: Some Internal Problem\n");
}


void get_PyFunc(lua_State* L, PyObject* pFunc)
{
	// creating new lua python callable
	size_t nbytes = sizeof(PyLua_PyCallable);
	PyLua_PyCallable* py_callable = (PyLua_PyCallable*)lua_newuserdata(L, nbytes);
	py_callable->function = pFunc;

	Py_INCREF(pFunc);

	lua_pushcclosure(L, PyLua_PyCallFunc, 1);
}
