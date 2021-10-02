#include "pylua.h"


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

static PyTypeObject pLuaTable_Type;

static PyObject* LuaError;


static PyObject* create_return(lua_State* L, int len)
{
	PyObject* pReturn = NULL;

	if (len == 0)
	{
		Py_INCREF(Py_None);
		pReturn = Py_None;
	}
	else if (len == 1)
	{
		pReturn = PyLua_LuaToPython(L, -1);
	}
	else
	{
		pReturn = PyTuple_New(len);
		int tmp = lua_gettop(L);

		for (int i = tmp - len, j = 0; i <= tmp; i++, j++)
		{
			PyTuple_SetItem(pReturn, j, PyLua_LuaToPython(L, i));
		}
	}

	return pReturn;
}

static PyObject* flip_boolean(PyObject* ob)
{
	if (ob == Py_True)
	{
		Py_DECREF(ob);
		ob = Py_False;
		Py_INCREF(ob);
	}
	else
	{
		Py_DECREF(ob);
		ob = Py_True;
		Py_INCREF(ob);
	}

	return ob;
}

static PyObject* next_LuaCoroutine(PyLua_LuaFunc* self)
{
	PyObject* pReturn = NULL;

	if (self->thread_terminated)
	{
		PyErr_SetNone(PyExc_StopIteration);
		return NULL;
	}

	lua_State* L = (lua_State*)self->lStack_prt;

	// make sure size of stact is the same at exit
	int stack_size = lua_gettop(L);

	// error message if function is not found
	int found_func = 0;

	// get list of variables from lua
	lua_getglobal(L, "_G");

	lua_pushnil(L);

	// iterate over them to find the function
	while (lua_next(L, -2))
	{
		if (lua_topointer(L, -1) == self->lFunc_prt)
		{
			// found the function
			found_func = 1;

			// to find number of return values
			int current_stack = lua_gettop(L);

			// copy function
			lua_pushvalue(L, -1);

			// number of yield values
			int yield_count = 0;

			lua_State* co = lua_tothread(L, -1);

			// call the function
			int resume_result = lua_resume(co, L, 0, &yield_count);

			if (resume_result == LUA_YIELD)
			{
				pReturn = create_return(co, yield_count);
				lua_pop(co, yield_count);

			}
			else if (resume_result == LUA_OK)
			{
				self->thread_terminated = 1;

				pReturn = create_return(co, yield_count);
				lua_pop(co, yield_count);

			}
			else
			{
				//return luaL_error(L, "Error: While calling the lua function.");

				PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(L, -1));
				return NULL;
			}
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}

	lua_pop(L, 1); // remove _G

	if (!found_func)
	{
		//return luaL_error(L, "Error: Lua function not found");

		if (self->is_luathread)
		{
			PyErr_SetString(LuaError, "Lua thread not found");
		}
		else
		{
			PyErr_SetString(LuaError, "Lua function not found");
		}
		return NULL;
	}

	// check stack size
	if (lua_gettop(L) != stack_size)
	{
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

	return pReturn;
}

static PyObject* iter_LuaCoroutine(PyLua_LuaFunc* self)
{
	if (!self->is_luathread)
	{
		PyErr_SetString(LuaError, "Not a lua thread");
		return NULL;
	}

	Py_INCREF(self);
	return self;
}


static PyObject* call_LuaFunc(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	if (self->is_luathread)
	{
		// raise error
		PyErr_SetString(LuaError, "Can not call lua thread type");
		return NULL;
	}

	if (kwargs)
	{
		// raise error
		PyErr_SetString(LuaError, "Lua function does not accept keyword arguments");
		return NULL;
	}

	lua_State* L = (lua_State*)self->lStack_prt;

	// make sure size of stact is the same at exit
	int stack_size = lua_gettop(L);

	// ensure space for all operations
	Py_ssize_t arg_len = PyTuple_Size(args);
	lua_checkstack(L, arg_len + 5);


	// error message if function is not found
	int found_func = 0;
	PyObject* pReturn = NULL;

	// get list of variables from lua
	lua_getglobal(L, "_G");

	lua_pushnil(L);

	// iterate over them to find the function
	while (lua_next(L, -2))
	{

		if (lua_topointer(L, -1) == self->lFunc_prt)
		{
			// found the function
			found_func = 1;

			// to find number of return values
			int current_stack = lua_gettop(L);

			// copy function
			lua_pushvalue(L, -1);

			for (int i = 0; i < arg_len; i++)
			{
				PyLua_PythonToLua(L, PyTuple_GetItem(args, i));
			}

			// call the function
			if (lua_pcall(L, arg_len, LUA_MULTRET, 0) != LUA_OK)
			{
				//return luaL_error(L, "Error: While calling the lua function.");

				PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(L, -1));
				return NULL;
			}

			int return_len = lua_gettop(L) - current_stack;

			pReturn = create_return(L, return_len);
			lua_pop(L, return_len);

		}

		lua_pop(L, 1);
	}

	lua_pop(L, 1); // remove _G

	if (!found_func)
	{
		//return luaL_error(L, "Error: Lua function not found");

		PyErr_SetString(LuaError, "Lua function not found");
		return NULL;
	}


	// check stack size
	if (lua_gettop(L) != stack_size)
	{
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

	return pReturn;

}

static PyObject* get_LuaFunc_Wrapper(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	if (kwargs)
	{
		// raise error
		PyErr_SetString(LuaError, "Lua function does not accept keyword arguments");
		return NULL;
	}

	uintptr_t a;
	uintptr_t b;
	int is_luathread;

	if (!PyArg_ParseTuple(args, "KKi", &a, &b, &is_luathread))
	{
		PyErr_SetString(LuaError, "Got wrong arguments to get_LuaFunc_Wrapper");
		return NULL;
	}

	self->lStack_prt = (void*)a;
	self->lFunc_prt = (void*)b;
	self->is_luathread = is_luathread;
	self->thread_terminated = 0;

	return 0;
}

static void stack_operation(lua_State* L, void* self, void* other)
{
	// make sure size of stact is +2 or +1 more then now at exit
	int stack_size = lua_gettop(L);

	// make sure of enough space in stack
	lua_checkstack(L, 7);

	// error message if table is not found
	int found_table1 = 0;
	int found_table2 = 0;

	// place holder for tables
	lua_pushnil(L);
	if (other)
	{
		lua_pushnil(L);
	}

	// get list of variables from lua
	lua_getglobal(L, "_G");

	lua_pushnil(L);

	// iterate over them to find the function
	while (lua_next(L, -2))
	{

		if (lua_topointer(L, -1) == self || lua_topointer(L, -1) == other)
		{
			// found the function
			if (found_table1)
			{
				found_table2 = 1;
			}
			else
			{
				found_table1 = 1;
			}

			lua_pushvalue(L, -1);

			// replace the top element with placeholder
			if (lua_topointer(L, -1) == self)
			{
				lua_replace(L, stack_size + 1);
			}
			else
			{
				lua_replace(L, stack_size + 2);
			}

		}

		lua_pop(L, 1);
	}

	lua_pop(L, 1); // remove _G


	if ((other) && (!found_table1 || !found_table2))
	{
		//return luaL_error(L, "Error: Lua function not found");

		PyErr_SetString(LuaError, "Lua tables not found");
		return NULL;
	}
	else if (!found_table1)
	{
		PyErr_SetString(LuaError, "Lua table not found");
		return NULL;
	}



	// check stack size
	if ((other) && (lua_gettop(L) != stack_size + 2))
	{
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}
	if ((!other) && lua_gettop(L) != stack_size + 1)
	{
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

}

static PyObject* getattr_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* attr)
{
	PyObject* pReturn = NULL;

	lua_State* L = (lua_State*)self->lStack_prt;

	// stack size matching
	int stack_size = lua_gettop(L);

	// get the two elements self and other
	stack_operation(L, self->lTable_prt, NULL);

	// attr to string
	PyObject* encodedString = PyUnicode_AsEncodedString(attr, "UTF-8", "strict");
	if (encodedString)
	{
		const char* attr_str = PyBytes_AsString(encodedString);
		if (attr_str)
		{
			lua_getfield(L, -1, attr_str);
			pReturn = PyLua_LuaToPython(L, -1);
		}
		Py_DECREF(encodedString);
	}

	lua_pop(L, 2);

	if (stack_size != lua_gettop(L))
	{
		// raise internal error
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

	return pReturn;
}

static PyObject* operation_LuaTable_base(PyLua_LuaTable* self, PyObject* other, const char* op)
{
	PyObject* pReturn;

	lua_State* L = (lua_State*)self->lStack_prt;
	lua_checkstack(L, 5);

	// stack size matching
	int stack_size = lua_gettop(L);

	// placeholder for add function
	lua_pushnil(L);

	// get the two elements self and other
	if (PyObject_IsInstance(other, (PyObject*)&pLuaTable_Type))
	{
		stack_operation(L, self->lTable_prt, ((PyLua_LuaTable*)other)->lTable_prt);
	}
	else
	{
		stack_operation(L, self->lTable_prt, NULL);
		PyLua_PythonToLua(L, other);
	}

	// get the add function
	lua_getmetatable(L, -2);
	lua_getfield(L, -1, op);
	lua_replace(L, stack_size + 1);
	lua_pop(L, 1);

	if (lua_gettop(L) == stack_size + 3)
	{
		if (lua_pcall(L, 2, 1, 0) != LUA_OK)
		{
			//return luaL_error(L, "Error: While calling the lua function.");

			PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(L, -1));
			return NULL;
		}

		pReturn = PyLua_LuaToPython(L, -1);
		lua_pop(L, 1);
	}
	else
	{
		// raise internal error
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

	return pReturn;
}

static PyObject* add_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__add");
}

static PyObject* sub_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__sub");
}

static PyObject* mul_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__mul");
}

static PyObject* div_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__div");
}

static PyObject* pow_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__pow");
}

static PyObject* mod_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__mod");
}

static PyObject* floordiv_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__idiv");
}

static PyObject* compare_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* other, int op)
{
	PyObject* pReturn;

	switch (op)
	{
	case 0:
		pReturn = operation_LuaTable_base(self, other, "__lt");
		break;
	case 1:
		pReturn = operation_LuaTable_base(self, other, "__le");
		break;
	case 2:
		pReturn = operation_LuaTable_base(self, other, "__eq");
		break;
	case 3:
		pReturn = operation_LuaTable_base(self, other, "__eq");
		pReturn = flip_boolean(pReturn);
		break;
	case 4:
		pReturn = operation_LuaTable_base(self, other, "__lt");
		pReturn = flip_boolean(pReturn);
		break;
	case 5:
		pReturn = operation_LuaTable_base(self, other, "__le");
		pReturn = flip_boolean(pReturn);
		break;
	default:
		pReturn = NULL;
		break;
	}

	return pReturn;
}

static PyObject* neg_LuaTable_Wrapper(PyLua_LuaTable* self)
{
	return operation_LuaTable_base(self, Py_None, "__unm");
}


static PyObject* get_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* args, PyObject* kwargs)
{
	if (kwargs)
	{
		// raise error
		PyErr_SetString(LuaError, "Lua function does not accept keyword arguments");
		return NULL;
	}

	uintptr_t a;
	uintptr_t b;

	if (!PyArg_ParseTuple(args, "KK", &a, &b))
	{
		PyErr_SetString(LuaError, "Got wrong arguments to get_LuaFunc_Wrapper");
		return NULL;
	}

	self->lStack_prt = a;
	self->lTable_prt = b;

	return 0;
}

static PyTypeObject pLuaFunc_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "pylua.lua_function_wrapper",
	.tp_doc = "pylua.lua_function_wrapper",
	.tp_basicsize = sizeof(PyLua_LuaFunc),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = get_LuaFunc_Wrapper,
	.tp_call = call_LuaFunc,
	.tp_iter = iter_LuaCoroutine,
	.tp_iternext = next_LuaCoroutine
};

static PyNumberMethods pLuaTable_NumberMethods = {
	.nb_add = add_LuaTable_Wrapper,
	.nb_subtract = sub_LuaTable_Wrapper,
	.nb_multiply = mul_LuaTable_Wrapper,
	.nb_true_divide = div_LuaTable_Wrapper,
	.nb_floor_divide = floordiv_LuaTable_Wrapper,
	.nb_power = pow_LuaTable_Wrapper,
	.nb_remainder = mod_LuaTable_Wrapper,
	.nb_negative = neg_LuaTable_Wrapper,
};

static PyTypeObject pLuaTable_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "pylua.lua_table_wrapper",
	.tp_doc = "pylua.lua_table_wrapper",
	.tp_basicsize = sizeof(PyLua_LuaTable),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = get_LuaTable_Wrapper,
	.tp_as_number = &pLuaTable_NumberMethods,
	.tp_richcompare = compare_LuaTable_Wrapper,
	.tp_getattro = getattr_LuaTable_Wrapper,
};


static struct PyModuleDef LUA_module = {
	PyModuleDef_HEAD_INIT,
	"pylua",
	"pylua doc",
	-1,
};

PyMODINIT_FUNC PyInit_pylua(void)
{
	PyObject* m;

	if (PyType_Ready(&pLuaFunc_Type) < 0)
	{
		return NULL;
	}

	m = PyModule_Create(&LUA_module);
	if (m == NULL)
	{
		return NULL;
	}


	Py_INCREF(&pLuaFunc_Type);
	if (PyModule_AddObject(m, "lua_function_wrapper", (PyObject*)&pLuaFunc_Type) < 0) {
		Py_DECREF(&pLuaFunc_Type);
		Py_DECREF(m);
		return NULL;
	}

	// ****************************
	if (PyType_Ready(&pLuaTable_Type) < 0)
	{
		return NULL;
	}

	Py_INCREF(&pLuaTable_Type);
	if (PyModule_AddObject(m, "lua_table_wrapper", (PyObject*)&pLuaTable_Type) < 0) {
		Py_DECREF(&pLuaTable_Type);
		Py_DECREF(m);
		return NULL;
	}
	// ****************************

	LuaError = PyErr_NewException("pylua.LuaError", NULL, NULL);
	if (PyModule_AddObject(m, "error", LuaError) < 0) {
		Py_XDECREF(LuaError);
		Py_CLEAR(LuaError);
		Py_DECREF(m);
		return NULL;
	}

	return m;
}
