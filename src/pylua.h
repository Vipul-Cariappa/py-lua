#ifndef PYLUA_H
#define PYLUA_H

#include <Python.h>
#include <lua.h>


// Debug build runtime checks
#ifdef DEBUG
#define SAVE_STACK_SIZE(L) size_t _ss = lua_gettop((L))
#define CHECK_STACK_SIZE(L, inc) assert((_ss + (inc)) == lua_gettop((L)))
#define CHECK_STACK_ZERO(L) assert(lua_gettop((L)) == 0)
#else
#define SAVE_STACK_SIZE(L)
#define CHECK_STACK_SIZE(L, inc)
#define CHECK_STACK_ZERO(L)
#endif // DEBUG


#define MEMROY_ERR "Memory Error"

// TODO: Error msg should also give info regarding which type could not get converted
#define LERR_CONVERT_PY_TO_LUA "Error: While converting from Python Type to Lua Type"
#define LERR_CONVERT_LUA_TO_PY "Error: While converting from Lua Type to Python Type"

#define PERR_CONVERT_PY_TO_LUA "Convertion from Python Type to Lua Type"
#define PERR_CONVERT_LUA_TO_PY "Convertion from Lua Type to Python Type"

#define ERR_CALL_PY "Error: While calling Python Function or Method"
#define ERR_CALL_LUA "Error while calling Lua Function or Thread\nLua Traceback:\n %s\n"

#define LERR_STOP_ITER "Error: Stop Iteration"

#define LERR_FUNC_NOT_FOUND "Error: The given Python Function could not be found for execution.\n\tThis may be because the function was cleared by the Garbage Collector."
#define PERR_FUNC_NOT_FOUND "The given Lua Function could not be found for execution.\n\tThis may be because the function was cleared by the Garbage Collector."

#define LERR_GET_ATTR "Error: Occurred when getting an Attribute from Python Object"
#define PERR_GET_ATTR "Error: Occurred when getting an Attribute from Lua Table"

// TODO: Some way to pass keyword may be required!
#define PERR_KWARG_LUA "Lua Function does not accept Python Keyword Arguments"

// common
PyObject* PyLua_LuaToPython(lua_State* L, int index);
int PyLua_PythonToLua(lua_State* L, PyObject* pItem);

typedef struct PyLua_PyModule {
	PyObject* module;
	int number;
	int loaded;
} PyLua_PyModule;

typedef struct PyLua_PyFunc {
	PyObject* function;
} PyLua_PyFunc;

typedef struct PyLua_PyIterator {
	PyObject* iterator;
} PyLua_PyIterator;

typedef struct PyLua_PyObject {
	PyObject* object;
} PyLua_PyObject;

typedef struct PyLua_LuaFunc {
	PyObject_HEAD
		int index;
	int is_luathread;
	int thread_terminated;
} PyLua_LuaFunc;

typedef struct PyLua_LuaTable {
	PyObject_HEAD
		int index;
} PyLua_LuaTable;

#endif // PYLUA_H
