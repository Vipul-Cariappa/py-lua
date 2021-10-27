#include "pylua.h"


PyTypeObject pLuaInstance_Type;
static PyObject* LuaError;

// lua_py.c
extern lua_State* cL;


// to be removed
void iterate_and_print_stack(lua_State* L)
{
	SAVE_STACK_SIZE(L);

	int stack_size = lua_gettop(L);

	for (int i = 1; i <= stack_size; i++)
	{
		lua_pushvalue(L, i);
		const char* value = lua_tostring(L, -1);
		if (!value)
		{
			printf("%i => %p\t Type: %s\n", i, lua_topointer(L, -2), luaL_typename(L, -2));
		}
		else
		{
			printf("%i => %s\n", i, value);
		}
		lua_pop(L, 1);
	}
	
	CHECK_STACK_SIZE(L, 0);

}
void iterate_and_print_table(lua_State* L, int index)
{
	SAVE_STACK_SIZE(L);
	// Push another reference to the table on top of the stack (so we know
	// where it is, and this function can work for negative, positive and
	// pseudo indices
	lua_pushvalue(L, index);
	// stack now contains: -1 => table
	lua_pushnil(L);
	// stack now contains: -1 => nil; -2 => table
	while (lua_next(L, -2))
	{
		// stack now contains: -1 => value; -2 => key; -3 => table
		// copy the key so that lua_tostring does not modify the original
		lua_pushvalue(L, -2);
		// stack now contains: -1 => key; -2 => value; -3 => key; -4 => table
		const char* key = lua_tostring(L, -1);
		const char* value = lua_tostring(L, -2);
		if (!value)
		{
			printf("%s => %p\t Type: %s\n", key, lua_topointer(L, -2), luaL_typename(L, -2));
		}
		else
		{
			printf("%s => %s\n", key, value);
		}
		// pop value + copy of key, leaving original key
		lua_pop(L, 2);
		// stack now contains: -1 => key; -2 => table
	}
	// stack now contains: -1 => table (when lua_next returns 0 it pops the key
	// but does not push anything.)
	// Pop table
	lua_pop(L, 1);
	
	CHECK_STACK_SIZE(L, 0);

}
// -------------


static PyObject* create_return(lua_State* L, int len)
{
	SAVE_STACK_SIZE(L);

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

	CHECK_STACK_SIZE(L, 0);

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

	// get function
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_rawgeti(cL, -1, self->index);

	//number of yield values
	int yield_count = 0;

	lua_State* co = lua_tothread(cL, -1);

	// call the function
	int resume_result = lua_resume(co, cL, 0, &yield_count);

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

		PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(cL, -1));
		return NULL;
	}

	lua_pop(cL, 2);

	// check stack size
	CHECK_STACK_ZERO(cL);

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

	// make sure size of stact is the same at exit
	int stack_size = lua_gettop(cL);

	// ensure space for all operations
	Py_ssize_t arg_len = PyTuple_Size(args);
	lua_checkstack(cL, arg_len + 5);

	// get function
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_rawgeti(cL, -1, self->index);

	// convert python variables to lua type
	for (int i = 0; i < arg_len; i++)
	{
		PyLua_PythonToLua(cL, PyTuple_GetItem(args, i));
	}

	// execute function
	if (lua_pcall(cL, arg_len, LUA_MULTRET, 0) != LUA_OK)
	{
		//return luaL_error(L, "Error: While calling the lua function.");

		PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(cL, -1));
		return NULL;
	}

	int return_len = lua_gettop(cL) - (stack_size + 1);

	PyObject* pReturn = create_return(cL, return_len);
	lua_pop(cL, return_len + 1);

	// check stack size
	CHECK_STACK_ZERO(cL);

	return pReturn;
}

static PyObject* gc_LuaFunc(PyLua_LuaFunc* self)
{
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	luaL_unref(cL, -1, self->index);
	lua_pop(cL, 1);

	CHECK_STACK_ZERO(cL);

	return 0;
}

static PyObject* get_LuaFunc_Wrapper(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	if (kwargs)
	{
		// raise error
		PyErr_SetString(LuaError, "Lua function does not accept keyword arguments");
		return NULL;
	}

	int index;
	int is_luathread;

	if (!PyArg_ParseTuple(args, "ii", &index, &is_luathread))
	{
		PyErr_SetString(LuaError, "Got wrong arguments to get_LuaFunc_Wrapper");
		return NULL;
	}

	self->index = index;
	self->is_luathread = is_luathread;
	self->thread_terminated = 0;

	//CHECK_STACK_ZERO(cL);

	return 0;
}


static PyObject* operation_LuaTable_base(PyLua_LuaTable* self, PyObject* other, const char* op)
{
	PyObject* pReturn;

	// stack size matching
	int stack_size = lua_gettop(cL);

	// get table
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_pushnil(cL); // placeholder for function (method)
	lua_rawgeti(cL, -2, self->index);

	// get function (method)
	lua_getfield(cL, -1, op);
	lua_replace(cL, -3);

	// get other element
	PyLua_PythonToLua(cL, other);

	//printf("Arg1: %p   Arg2: %p, Op: %s\n", lua_topointer(cL, -2), lua_topointer(cL, -1), op);

	// call function
	if (lua_pcall(cL, 2, 1, 0) != LUA_OK)
	{
		//return luaL_error(L, "Error: While calling the lua function.");

		//TODO: manage stack before raising error

		PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(cL, -1));
		return NULL;
	}

	pReturn = PyLua_LuaToPython(cL, -1);
	lua_pop(cL, 2);	// result and registry

	CHECK_STACK_ZERO(cL);

	return pReturn;
}

static PyObject* getelem_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* pKey)
{
	PyObject* pReturn = NULL;

	// to string
	PyObject* pStr = PyObject_Str(pKey);
	PyObject* encodedString = PyUnicode_AsEncodedString(pKey, "UTF-8", "strict");
	const char* key_str = NULL;

	// string
	if (encodedString)
	{
		key_str = PyBytes_AsString(encodedString);
		if (!key_str)
		{
			// raise error
			return NULL;
		}
	}

	// get table
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_rawgeti(cL, -2, self->index);

	// get item
	lua_getfield(cL, -1, key_str);
	pReturn = PyLua_LuaToPython(cL, -1);

	lua_pop(cL, 3);

	CHECK_STACK_ZERO(cL);

	Py_DECREF(pStr);
	Py_DECREF(encodedString);
	return pReturn;
}

static int setelem_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* pKey, PyObject* pValue)
{
	// to string
	PyObject* pStr = PyObject_Str(pKey);
	PyObject* encodedString = PyUnicode_AsEncodedString(pKey, "UTF-8", "strict");
	const char* key_str = NULL;

	// string
	if (encodedString)
	{
		key_str = PyBytes_AsString(encodedString);
		if (!key_str)
		{
			// raise error
			return -1;
		}
	}

	// get table
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_rawgeti(cL, -2, self->index);

	// set value
	// handle del
	if (!pValue)
	{
		lua_pushnil(cL);
		lua_setfield(cL, -2, key_str);
	}

	PyLua_PythonToLua(cL, pValue);
	lua_setfield(cL, -2, key_str);

	lua_pop(cL, 2);

	Py_DECREF(pStr);
	Py_DECREF(encodedString);
	
	CHECK_STACK_ZERO(cL);
	
	return 0;
}

static int wrapper_around_method(lua_State* L)
{
	int stack_size = lua_gettop(L);
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_insert(L, 1);
	lua_pushvalue(L, lua_upvalueindex(2));
	lua_insert(L, 1);

	assert(stack_size + 2 == lua_gettop(L));

	// execute function
	if (lua_pcall(L, stack_size + 1, LUA_MULTRET, 0) != LUA_OK)
	{
		//return luaL_error(L, "Error: While calling the lua function.");

		PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(cL, -1));
		return NULL;
	}

	int return_len = lua_gettop(L);
	assert(lua_gettop(L) == return_len);

	return return_len;
}

static PyObject* getattr_LuaInstance_Wrapper(PyLua_LuaTable* self, char* attr)
{
	PyObject* pReturn;

	// get table
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_rawgeti(cL, -1, self->index);

	// get item
	lua_getfield(cL, -1, attr);

	// Use LuaToPython only if the attribute is not function (i.e method)
	if (lua_type(cL, -1) != LUA_TFUNCTION)
	{
		pReturn = PyLua_LuaToPython(cL, -1);
		lua_pop(cL, 3);
	}
	// else wrap it around wrapper_around_method function and set closures values to be.
	else 
	{
		lua_pushcclosure(cL, wrapper_around_method, 2);
		pReturn = PyLua_LuaToPython(cL, -1);
		lua_pop(cL, 2);
	}

	CHECK_STACK_ZERO(cL);

	return pReturn;
}

static int setattr_LuaInstance_Wrapper(PyLua_LuaTable* self, char* attr, PyObject* pValue)
{
	// get table
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_rawgeti(cL, -1, self->index);

	// set value
	// handle del
	if (!pValue)
	{
		lua_pushnil(cL);
		lua_setfield(cL, -2, attr);
	}

	PyLua_PythonToLua(cL, pValue);
	lua_setfield(cL, -2, attr);

	lua_pop(cL, 2);

	CHECK_STACK_ZERO(cL);

	return 0;
}

static PyObject* call_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* args, PyObject* kwargs)
{
	if (kwargs)
	{
		// raise error
		PyErr_SetString(LuaError, "Lua function does not accept keyword arguments");
		return NULL;
	}

	PyObject* pReturn;

	// get table
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_pushnil(cL); // placeholder for function (method)
	lua_rawgeti(cL, -2, self->index);

	// get initialiser
	lua_getfield(cL, -1, "__call");
	lua_replace(cL, -3);

	// python args to lua
	Py_ssize_t arg_len = PyTuple_Size(args);

	for (int i = 0; i < arg_len; i++)
	{
		PyLua_PythonToLua(cL, PyTuple_GetItem(args, i));
	}

	// call function
	if (lua_pcall(cL, arg_len + 1, 1, 0) != LUA_OK)
	{
		//return luaL_error(L, "Error: While calling the lua function.");

		PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(cL, -1));
		return NULL;
	}

	pReturn = PyLua_LuaToPython(cL, -1);
	lua_pop(cL, 2); // result and registry

	CHECK_STACK_ZERO(cL);

	return pReturn;
}

static PyObject* concat_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__concat");
}

static PyObject* add_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__add");
}

static PyObject* sub_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__sub");
}

static PyObject* mul_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__mul");
}

static PyObject* div_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__div");
}

static PyObject* pow_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__pow");
}

static PyObject* mod_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__mod");
}

static PyObject* floordiv_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__idiv");
}

static PyObject* band_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__band");
}

static PyObject* bor_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__bor");
}

static PyObject* bxor_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__bxor");
}

static PyObject* lshift_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__shl");
}

static PyObject* rshift_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	return operation_LuaTable_base(self, other, "__shr");
}

static PyObject* bnot_LuaInstance_Wrapper(PyLua_LuaTable* self)
{
	return operation_LuaTable_base(self, Py_None, "__bnot");
}


static PyObject* compare_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other, int op)
{
	PyObject* pReturn;

	switch (op)
	{
	case Py_LT:
		pReturn = operation_LuaTable_base(self, other, "__lt");
		break;
	case Py_LE:
		pReturn = operation_LuaTable_base(self, other, "__le");
		break;
	case Py_EQ:
		pReturn = operation_LuaTable_base(self, other, "__eq");
		break;
	case Py_NE:
		pReturn = operation_LuaTable_base(self, other, "__eq");
		pReturn = flip_boolean(pReturn);
		break;
	case Py_GT:
		pReturn = operation_LuaTable_base(self, other, "__lt");
		pReturn = flip_boolean(pReturn);
		break;
	case Py_GE:
		pReturn = operation_LuaTable_base(self, other, "__le");
		pReturn = flip_boolean(pReturn);
		break;
	default:
		pReturn = NULL;
		break;
	}

	CHECK_STACK_ZERO(cL);

	return pReturn;
}

static PyObject* string_LuaInstance_Wrapper(PyLua_LuaTable* self)
{
	return operation_LuaTable_base(self, Py_None, "__tostring");
}

static PyObject* neg_LuaInstance_Wrapper(PyLua_LuaTable* self)
{
	return operation_LuaTable_base(self, Py_None, "__unm");
}

static Py_ssize_t len_LuaInstance_Wrapper(PyLua_LuaTable* self)
{
	Py_ssize_t x;

	PyObject* pObj = operation_LuaTable_base(self, Py_None, "__len");
	if (!pObj)
	{
		return -1;
	}

	PyObject* pReturn = PyNumber_Long(pObj);
	if (pReturn)
	{
		x = PyNumber_AsSsize_t(pReturn, NULL);
		if (x < 0)
		{
			PyErr_Format(LuaError, "__len function did not return non negative interger");
		}
	}
	else
	{
		x = -1;
		PyErr_Format(LuaError, "__len function did not return integer");
	}

	Py_DECREF(pObj);
	Py_XDECREF(pReturn);
	return x;
}

static PyObject* call_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* args, PyObject* kwargs)
{
	PyObject* pReturn;

	// stack size matching
	int stack_size = lua_gettop(cL);

	// get table
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_pushnil(cL); // placeholder for function (method)
	lua_rawgeti(cL, -2, self->index);

	// get initialiser
	lua_getfield(cL, -1, "new");
	lua_replace(cL, stack_size + 2);


	// python args to lua
	Py_ssize_t arg_len = PyTuple_Size(args);

	for (int i = 0; i < arg_len; i++)
	{
		PyLua_PythonToLua(cL, PyTuple_GetItem(args, i));
	}

	// call function
	if (lua_pcall(cL, arg_len + 1, 1, 0) != LUA_OK)
	{
		//return luaL_error(L, "Error: While calling the lua function.");

		PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(cL, -1));
		return NULL;
	}

	pReturn = PyLua_LuaToPython(cL, -1);
	lua_pop(cL, 2); // result and registry

	CHECK_STACK_ZERO(cL);

	return pReturn;
}

static PyObject* get_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* args, PyObject* kwargs)
{
	if (kwargs)
	{
		// raise error
		PyErr_SetString(LuaError, "Lua function does not accept keyword arguments");
		return NULL;
	}

	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
	{
		PyErr_SetString(LuaError, "Got wrong arguments to get_LuaFunc_Wrapper");
		return NULL;
	}

	self->index = index;

	return 0;
}

static PyObject* gc_LuaTable(PyLua_LuaTable* self)
{
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	luaL_unref(cL, -1, self->index);
	lua_pop(cL, 1);

	CHECK_STACK_ZERO(cL);

	return 0;
}

static PyTypeObject pLuaFunc_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "pylua.lua_function_wrapper",
	.tp_doc = "pylua.lua_function_wrapper",
	.tp_basicsize = sizeof(PyLua_LuaFunc),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = &PyType_GenericNew,
	.tp_init = &get_LuaFunc_Wrapper,
	.tp_call = &call_LuaFunc,
	.tp_iter = &iter_LuaCoroutine,
	.tp_iternext = &next_LuaCoroutine,
	.tp_finalize = &gc_LuaFunc,
};

static PyMappingMethods pLuaInstance_MappingMethods = {
	.mp_length = &len_LuaInstance_Wrapper,
};

static PyMappingMethods pLuaTable_MappingMethods = {
	.mp_length = &len_LuaInstance_Wrapper,
	.mp_subscript = &getelem_LuaTable_Wrapper,
	.mp_ass_subscript = &setelem_LuaTable_Wrapper,
};

static PyNumberMethods pLuaInstance_NumberMethods = {
	.nb_add = &add_LuaInstance_Wrapper,
	.nb_subtract = &sub_LuaInstance_Wrapper,
	.nb_multiply = &mul_LuaInstance_Wrapper,
	.nb_true_divide = &div_LuaInstance_Wrapper,
	.nb_floor_divide = &floordiv_LuaInstance_Wrapper,
	.nb_power = &pow_LuaInstance_Wrapper,
	.nb_remainder = &mod_LuaInstance_Wrapper,
	.nb_negative = &neg_LuaInstance_Wrapper,
	.nb_lshift = &lshift_LuaInstance_Wrapper,
	.nb_rshift = &rshift_LuaInstance_Wrapper,
	.nb_and = &band_LuaInstance_Wrapper,
	.nb_or = &bor_LuaInstance_Wrapper,
	.nb_xor = &bxor_LuaInstance_Wrapper,
	.nb_invert = &bnot_LuaInstance_Wrapper,
	.nb_matrix_multiply = &concat_LuaInstance_Wrapper,
};

PyTypeObject pLuaInstance_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "pylua.lua_instance_wrapper",
	.tp_doc = "pylua.lua_instance_wrapper",
	.tp_basicsize = sizeof(PyLua_LuaTable),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = &get_LuaTable_Wrapper,
	.tp_as_number = &pLuaInstance_NumberMethods,
	.tp_as_mapping = &pLuaInstance_MappingMethods,
	.tp_richcompare = &compare_LuaInstance_Wrapper,
	.tp_getattr = &getattr_LuaInstance_Wrapper,
	.tp_setattr = &setattr_LuaInstance_Wrapper,
	.tp_call = &call_LuaInstance_Wrapper,
	.tp_str = &string_LuaInstance_Wrapper,
	.tp_finalize = &gc_LuaTable,
};

PyTypeObject pLuaTable_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "pylua.lua_table_wrapper",
	.tp_doc = "pylua.lua_table_wrapper",
	.tp_basicsize = sizeof(PyLua_LuaTable),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = get_LuaTable_Wrapper,
	.tp_as_mapping = &pLuaTable_MappingMethods,
	.tp_call = call_LuaTable_Wrapper,
	.tp_finalize = &gc_LuaTable,
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

	if (PyType_Ready(&pLuaInstance_Type) < 0)
	{
		return NULL;
	}

	Py_INCREF(&pLuaInstance_Type);
	if (PyModule_AddObject(m, "lua_instance_wrapper", (PyObject*)&pLuaInstance_Type) < 0) {
		Py_DECREF(&pLuaInstance_Type);
		Py_DECREF(m);
		return NULL;
	}

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

	LuaError = PyErr_NewException("pylua.LuaError", NULL, NULL);
	if (PyModule_AddObject(m, "error", LuaError) < 0) {
		Py_XDECREF(LuaError);
		Py_CLEAR(LuaError);
		Py_DECREF(m);
		return NULL;
	}

	return m;
}
