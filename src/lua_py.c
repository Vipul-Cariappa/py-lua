#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// bindings.c
int set(lua_State* L);
int get(lua_State* L);
int PY_load(lua_State* L);
int PY_unload(lua_State* L);

// callable.c
int PYCallable_unload(lua_State* L);
int call(lua_State* L);

int add(lua_State* L)
{
	if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
	{
		double result = luaL_checknumber(L, 1) + luaL_checknumber(L, 2);
		lua_pushnumber(L, result);
	}
	return 1;
}

int sub(lua_State* L)
{
	if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
	{
		double result = luaL_checknumber(L, 1) - luaL_checknumber(L, 2);
		lua_pushnumber(L, result);
	}
	return 1;
}


int py_print(lua_State* L) 
{
	if (Py_IsInitialized())
	{
		PyRun_SimpleString("print('Hello World From Python!', flush=True)");
	}

	return 0;
}

// ------------------------------------------------------
typedef struct NumArray {
	int size;
	double values[1];  /* variable part */
} NumArray;

static NumArray* checkarray(lua_State* L) {
	void* ud = luaL_checkudata(L, 1, "LuaBook.array");
	luaL_argcheck(L, ud != NULL, 1, "`array' expected");
	return (NumArray*)ud;
}

static double* getelem(lua_State* L) {
	NumArray* a = checkarray(L);
	int index = luaL_checkinteger(L, 2);

	luaL_argcheck(L, 1 <= index && index <= a->size, 2,
		"index out of range");

	/* return element address */
	return &a->values[index - 1];
}

static int newarray(lua_State* L) {
	int n = luaL_checkinteger(L, 1);
	size_t nbytes = sizeof(NumArray) + (n - 1) * sizeof(double);
	NumArray* a = (NumArray*)lua_newuserdata(L, nbytes);

	luaL_getmetatable(L, "LuaBook.array");
	lua_setmetatable(L, -2);

	a->size = n;
	return 1;  /* new userdatum is already on the stack */
}

static int setarray(lua_State* L) {
	double newvalue = luaL_checknumber(L, 3);
	*getelem(L) = newvalue;
	return 0;
}

static int getarray(lua_State* L) {
	lua_pushnumber(L, *getelem(L));
	return 1;
}

static int getsize(lua_State* L) {
	NumArray* a = checkarray(L);
	lua_pushnumber(L, a->size);
	return 1;
}

int array2string(lua_State* L) {
	NumArray* a = checkarray(L);
	lua_pushfstring(L, "array(%d)", a->size);
	return 1;
}


static int delarray(lua_State* L)
{
	printf("I am able to delete!\n");
	return 0;
}

static const struct luaL_Reg arraylib_f[] = {
	  {"new", newarray},
	  {NULL, NULL}
};

static const struct luaL_Reg arraylib_m[] = {
	{"__tostring", array2string},
	{"set", setarray},
	{"get", getarray},
	{"size", getsize},
	//{"__gc", delarray},
	{NULL, NULL}
};

int get_get(lua_State* L)
{
	int n = 1000000;
	size_t nbytes = sizeof(NumArray) + (n - 1) * sizeof(double);
	NumArray* a = (NumArray*)lua_newuserdata(L, nbytes);

	luaL_getmetatable(L, "LuaBook.array");
	lua_setmetatable(L, -2);

	a->size = n;

	for (int i = 0; i < n; i++)
	{
		a->values[i] = i;
	}

	//lua_pop(L, 1);

	int result = lua_getfield(L, 1, "get");
	if (result == LUA_TFUNCTION)
	{
		printf("Evering thing working fine\n");
	}

	return 2;
}

// -------------------------------------------------------
static const struct luaL_Reg PY_lib[] = {
	{"PyLoad", PY_load},
	{"PyUnLoad", PY_unload},
	{NULL, NULL}
};

static const struct luaL_Reg PY_callable[] = {
	{"call", call},
	{NULL, NULL}
};

// should be removed
int tmp(lua_State* L)
{
	return 0;
}

__declspec(dllexport) int luaopen_pylua(lua_State* L) {
	lua_register(L, "add", add);
	lua_register(L, "sub", sub);
	lua_register(L, "py_print", py_print);
	lua_register(L, "get_get", get_get);

	// python module
	luaL_newmetatable(L, "Python.Module");
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, PY_unload);
	lua_settable(L, -3);
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, get);
	lua_settable(L, -3);
	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, set);
	lua_settable(L, -3);
	luaL_setfuncs(L, PY_lib, 0);
	lua_setglobal(L, "Python");

	// setting lua for python callable
	luaL_newmetatable(L, "Python.Callable");
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, PYCallable_unload);
	lua_settable(L, -3);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);  /* pushes the metatable */
	lua_settable(L, -3);  /* metatable.__index = metatable */
	lua_pushstring(L, "call");
	lua_pushcfunction(L, call);
	lua_settable(L, -3);


	luaL_newmetatable(L, "LuaBook.array");

	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, delarray);
	lua_settable(L, -3);
	
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);  /* pushes the metatable */
	lua_settable(L, -3);  /* metatable.__index = metatable */

	luaL_setfuncs(L, arraylib_m, 0);

	luaL_setfuncs(L, arraylib_f, 0);
	lua_setglobal(L, "array");
	return 3;
}
