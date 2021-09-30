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

	char* str_replace(char* orig, char* rep, char* with);

#endif // !pylua
