#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


#ifndef pylua

	PyObject* PyLua_LuaToPython(lua_State* L, int index);
	int PyLua_PythonToLua(lua_State* L, PyObject* pItem);

	typedef struct PyLua_PyCallable {
		PyObject* function;
	} PyLua_PyCallable;

	typedef struct PyLua_PyIterator {
		PyObject* iterator;
	} PyLua_PyIterator;

#endif // !pylua
