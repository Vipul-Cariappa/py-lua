#include "pylua.h"


PyTypeObject pLuaInstance_Type;
static PyObject* LuaError;

// lua_py.c
extern lua_State* cL;

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

	// make sure size of stact is the same at exit
	int stack_size = lua_gettop(cL);

	// get function
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_getfield(cL, -1, self->name);

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
	if (lua_gettop(cL) != stack_size)
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

	// make sure size of stact is the same at exit
	int stack_size = lua_gettop(cL);

	// ensure space for all operations
	Py_ssize_t arg_len = PyTuple_Size(args);
	lua_checkstack(cL, arg_len + 5);

	// get function
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_getfield(cL, -1, self->name);
	
	// execute function
	if (lua_pcall(cL, arg_len, LUA_MULTRET, 0) != LUA_OK)
	{
		//return luaL_error(L, "Error: While calling the lua function.");

		PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(cL, -1));
		return NULL;
	}

	int return_len = lua_gettop(cL) - stack_size + 1 ;

	PyObject* pReturn = create_return(cL, return_len);
	lua_pop(cL, return_len);

	// check stack size
	if (lua_gettop(cL) != stack_size)
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

	char* name;
	int is_luathread;

	if (!PyArg_ParseTuple(args, "si", &name, &is_luathread))
	{
		PyErr_SetString(LuaError, "Got wrong arguments to get_LuaFunc_Wrapper");
		return NULL;
	}

	self->name = name;
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

static PyObject* operation_LuaTable_base(PyLua_LuaTable* self, PyObject* other, const char* op)
{
	PyObject* pReturn;

	lua_State* L = (lua_State*)self->lStack_prt;
	lua_checkstack(L, 5);

	// stack size matching
	int stack_size = lua_gettop(L);

	// placeholder for function
	lua_pushnil(L);

	// get the two elements self and other
	if (PyObject_IsInstance(other, (PyObject*)&pLuaInstance_Type))
	{
		stack_operation(L, self->lTable_prt, ((PyLua_LuaTable*)other)->lTable_prt);
	}
	else
	{
		stack_operation(L, self->lTable_prt, NULL);
		PyLua_PythonToLua(L, other);
	}

	// get the function
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

	lua_State* L = (lua_State*)self->lStack_prt;

	// stack size matching
	int stack_size = lua_gettop(L);

	// get the two elements self and other
	stack_operation(L, self->lTable_prt, NULL);

	// get item
	lua_getfield(L, -1, key_str);
	pReturn = PyLua_LuaToPython(L, -1);

	lua_pop(L, 2);

	if (stack_size != lua_gettop(L))
	{
		// raise internal error
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

	Py_DECREF(pStr);
	Py_DECREF(encodedString);
	return pReturn;
}

static int setelem_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* pKey, PyObject* pValue)
{
	lua_State* L = (lua_State*)self->lStack_prt;

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

	// stack size matching
	int stack_size = lua_gettop(L);

	// get the two elements self and other
	stack_operation(L, self->lTable_prt, NULL);

	if (!key_str)
	{
		// delete the value at the given key
		lua_pushnil(L);
		lua_setfield(L, -2, key_str);

		Py_DECREF(pStr);
		Py_DECREF(encodedString);
		return 0;
	}

	// set attr
	PyLua_PythonToLua(L, pValue);
	lua_setfield(L, -2, key_str);

	lua_pop(L, 1);

	if (stack_size != lua_gettop(L))
	{
		// raise internal error
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

	Py_DECREF(pStr);
	Py_DECREF(encodedString);
	return 0;
}

static PyObject* getattr_LuaInstance_Wrapper(PyLua_LuaTable* self, char* attr)
{
	PyObject* pReturn = NULL;

	lua_State* L = (lua_State*)self->lStack_prt;

	// stack size matching
	int stack_size = lua_gettop(L);

	// get the two elements self and other
	stack_operation(L, self->lTable_prt, NULL);

	// get item
	lua_getfield(L, -1, attr);
	pReturn = PyLua_LuaToPython(L, -1);

	lua_pop(L, 2);

	if (stack_size != lua_gettop(L))
	{
		// raise internal error
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

	return pReturn;
}

static int setattr_LuaInstance_Wrapper(PyLua_LuaTable* self, char* attr, PyObject* value)
{
	lua_State* L = (lua_State*)self->lStack_prt;

	// stack size matching
	int stack_size = lua_gettop(L);

	// get the two elements self and other
	stack_operation(L, self->lTable_prt, NULL);

	if (!value)
	{
		// delete the value at the given attr
		lua_pushnil(L);
		lua_setfield(L, -2, attr);
		return 0;
	}

	// set attr
	PyLua_PythonToLua(L, value);
	lua_setfield(L, -2, attr);

	lua_pop(L, 1);

	if (stack_size != lua_gettop(L))
	{
		// raise internal error
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

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

	lua_State* L = (lua_State*)self->lStack_prt;
	lua_checkstack(L, 5);

	// stack size matching
	int stack_size = lua_gettop(L);

	// placeholder for call function
	lua_pushnil(L);

	// get self & function
	stack_operation(L, self->lTable_prt, NULL);
	lua_getmetatable(L, -1);
	lua_getfield(L, -1, "__call");
	lua_replace(L, stack_size + 1);
	lua_pop(L, 1);

	Py_ssize_t arg_len = PyTuple_Size(args);

	// python to lua
	for (int i = 0; i < arg_len; i++)
	{
		PyLua_PythonToLua(L, PyTuple_GetItem(args, i));
	}

	// call the function
	if (lua_pcall(L, arg_len + 1, LUA_MULTRET, 0) != LUA_OK)
	{
		//return luaL_error(L, "Error: While calling the lua function.");

		PyErr_Format(LuaError, "\nError raise while executing lua\nLua Traceback:\n %s\n", lua_tostring(L, -1));
		return NULL;
	}

	int return_len = lua_gettop(L) - stack_size;

	pReturn = create_return(L, return_len);
	lua_pop(L, return_len);

	// check stack size
	if (lua_gettop(L) != stack_size)
	{
		fprintf(stderr, "Error: Stack size not same.\n\tPlease Report this Issue");
		exit(-1);
	}

	return pReturn;
}

static PyObject* add_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other)
{
	PyObject* pReturn;

	lua_State* L = (lua_State*)self->lStack_prt;
	lua_checkstack(L, 5);

	// stack size matching
	int stack_size = lua_gettop(L);

	// placeholder for function
	lua_pushnil(L);

	// get the two elements self and other
	if (PyObject_IsInstance(other, (PyObject*)&pLuaInstance_Type))
	{
		stack_operation(L, self->lTable_prt, ((PyLua_LuaTable*)other)->lTable_prt);
	}
	else
	{
		stack_operation(L, self->lTable_prt, NULL);
		PyLua_PythonToLua(L, other);
	}

	// get the function
	lua_getmetatable(L, -2);
	if (lua_getfield(L, -1, "__add") != LUA_TFUNCTION)
	{
		lua_pop(L, 1);
		lua_getfield(L, -1, "__concat");
	}
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

	lua_State* L = self->lStack_prt;

	lua_checkstack(L, 5);

	// stack size matching
	int stack_size = lua_gettop(L);

	// placeholder for function
	lua_pushnil(L);

	// get table
	stack_operation(L, self->lTable_prt, NULL);

	// get the function
	lua_getfield(L, -1, "new"); // make this dynamic from kwargs
	lua_replace(L, stack_size + 1);


	if (lua_gettop(L) == stack_size + 2)
	{
		// python args to lua
		Py_ssize_t arg_len = PyTuple_Size(args);

		for (int i = 0; i < arg_len; i++)
		{
			PyLua_PythonToLua(L, PyTuple_GetItem(args, i));
		}

		if (lua_pcall(L, arg_len + 1, 1, 0) != LUA_OK)
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

static PyMappingMethods pLuaInstance_MappingMethods = {
	.mp_length = len_LuaInstance_Wrapper,
};

static PyMappingMethods pLuaTable_MappingMethods = {
	.mp_length = len_LuaInstance_Wrapper,
	.mp_subscript = getelem_LuaTable_Wrapper,
	.mp_ass_subscript = setelem_LuaTable_Wrapper,
};

static PyNumberMethods pLuaInstance_NumberMethods = {
	.nb_add = add_LuaInstance_Wrapper,
	.nb_subtract = sub_LuaInstance_Wrapper,
	.nb_multiply = mul_LuaInstance_Wrapper,
	.nb_true_divide = div_LuaInstance_Wrapper,
	.nb_floor_divide = floordiv_LuaInstance_Wrapper,
	.nb_power = pow_LuaInstance_Wrapper,
	.nb_remainder = mod_LuaInstance_Wrapper,
	.nb_negative = neg_LuaInstance_Wrapper,
	.nb_lshift = lshift_LuaInstance_Wrapper,
	.nb_rshift = rshift_LuaInstance_Wrapper,
	.nb_and = band_LuaInstance_Wrapper,
	.nb_or = bor_LuaInstance_Wrapper,
	.nb_xor = bxor_LuaInstance_Wrapper,
	.nb_invert = bnot_LuaInstance_Wrapper,
};

extern PyTypeObject pLuaInstance_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "pylua.lua_instance_wrapper",
	.tp_doc = "pylua.lua_instance_wrapper",
	.tp_basicsize = sizeof(PyLua_LuaTable),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = get_LuaTable_Wrapper,
	.tp_as_number = &pLuaInstance_NumberMethods,
	.tp_as_mapping = &pLuaInstance_MappingMethods,
	.tp_richcompare = compare_LuaInstance_Wrapper,
	.tp_getattr = getattr_LuaInstance_Wrapper,
	.tp_setattr = setattr_LuaInstance_Wrapper,
	.tp_call = call_LuaInstance_Wrapper,
	.tp_str = string_LuaInstance_Wrapper,
};

extern PyTypeObject pLuaTable_Type = {
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
