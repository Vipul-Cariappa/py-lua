#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <assert.h>

#ifndef pylua
#define pylua

	PyObject* PyLua_LuaToPython(lua_State* L, int index);
	int PyLua_PythonToLua(lua_State* L, PyObject* pItem);

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

	char* str_replace(char* orig, char* rep, char* with);

	#ifdef DEBUG
		#define SAVE_STACK_SIZE(L) size_t _ss = lua_gettop((L))
		#define CHECK_STACK_SIZE(L, inc) assert((_ss + (inc)) == lua_gettop((L)))
		#define CHECK_STACK_ZERO(L) assert(lua_gettop((L)) == 0)
	#endif // DEBUG



#endif // !pylua
