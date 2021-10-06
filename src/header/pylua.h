#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


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
		void* lStack_prt;
		void* lFunc_prt;
		int is_luathread;
		int thread_terminated;
	} PyLua_LuaFunc;

	typedef struct PyLua_LuaTable {
		PyObject_HEAD
		void* lStack_prt;
		void* lTable_prt;
	} PyLua_LuaTable;

	char* str_replace(char* orig, char* rep, char* with);

#endif // !pylua
