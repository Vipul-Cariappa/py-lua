#include "py_lua.h"


int PyLua_LuaLoadedModuleCount = 0;


PyMODINIT_FUNC PyInit_pylua(void)
{
	if (DrivingLang == -1)
	{
		DrivingLang = 1;

		// initilise lua interpreter
		nL = luaL_newstate();
		luaL_openlibs(nL);
		
		// load pylua module onto lua interpreter
		lua_pushcfunction(nL, &luaopen_pylua);
		if (lua_pcall(nL, 0, 0, 0) != LUA_OK)
		{
			// raise error
			PyErr_SetString(PyExc_ImportError, "Unable to load pylua module onto lua interpreter");
			lua_close(nL);
			return NULL;
		}
	}

	PyObject* m;

	m = PyModule_Create(&LUA_module);
	if (m == NULL)
	{
		return NULL;
	}

	if (PyType_Ready(&pLuaModule_Type) < 0)
	{
		return NULL;
	}

	Py_INCREF(&pLuaModule_Type);
	if (PyModule_AddObject(m, "LuaModule", (PyObject*)&pLuaModule_Type) < 0) {
		Py_DECREF(&pLuaModule_Type);
		Py_DECREF(m);
		return NULL;
	}
	
	if (PyType_Ready(&pLuaFunc_Type) < 0)
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

	pPylua_Module = m;

	return m;
}


static PyTypeObject pLuaFunc_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "pylua.lua_function_wrapper",
	.tp_doc = "pylua.lua_function_wrapper",
	.tp_basicsize = sizeof(PyLua_LuaFunc),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = &PyType_GenericNew,
	.tp_init = (initproc)init_LuaFunc_Wrapper,
	.tp_call = (ternaryfunc)call_LuaFunc,
	.tp_iter = (getiterfunc)iter_LuaCoroutine,
	.tp_iternext = (iternextfunc)next_LuaCoroutine,
	.tp_finalize = (destructor)gc_LuaFunc,
};

// TODO: implement convertion to dict, list and iteration for Lua Tables and Instances

PyTypeObject pLuaTable_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "pylua.lua_table_wrapper",
	.tp_doc = "pylua.lua_table_wrapper",
	.tp_basicsize = sizeof(PyLua_LuaTable),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)init_LuaTable_Wrapper,
	.tp_as_mapping = &pLuaTable_MappingMethods,
	.tp_call = (ternaryfunc)call_LuaTable_Wrapper,
	.tp_iter = (getiterfunc)iter_LuaTable,
	.tp_finalize = (destructor)gc_LuaTable,
};

PyTypeObject pLuaInstance_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "pylua.lua_instance_wrapper",
	.tp_doc = "pylua.lua_instance_wrapper",
	.tp_basicsize = sizeof(PyLua_LuaTable),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)init_LuaTable_Wrapper,
	.tp_as_number = &pLuaInstance_NumberMethods,
	.tp_as_mapping = &pLuaInstance_MappingMethods,
	.tp_richcompare = (richcmpfunc)compare_LuaInstance_Wrapper,
	.tp_getattr = (getattrfunc)getattr_LuaInstance_Wrapper,
	.tp_setattr = (setattrfunc)setattr_LuaInstance_Wrapper,
	.tp_call = (ternaryfunc)call_LuaInstance_Wrapper,
	.tp_iter = (getiterfunc)iter_LuaTable,
	.tp_str = (reprfunc)string_LuaInstance_Wrapper,
	.tp_finalize = (destructor)gc_LuaTable,
};

static PyTypeObject pLuaModule_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "pylua.LuaLoad",
	.tp_doc = "pylua.LuaLoad",
	.tp_basicsize = sizeof(PyLua_LuaModule),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)PyLua_LuaInit,
	.tp_getattr = (getattrfunc)PyLua_LuaGet,
	.tp_setattr = (setattrfunc)PyLua_LuaSet,
	.tp_finalize = (destructor)PyLua_LuaGC,
};


static PyMappingMethods pLuaTable_MappingMethods = {
	.mp_length = (lenfunc)len_LuaInstance_Wrapper,
	.mp_subscript = (binaryfunc)getelem_LuaTable_Wrapper,
	.mp_ass_subscript = (objobjargproc)setelem_LuaTable_Wrapper,
};


static PyMappingMethods pLuaInstance_MappingMethods = {
	.mp_length = (lenfunc)len_LuaInstance_Wrapper,
};

static PyNumberMethods pLuaInstance_NumberMethods = {
	.nb_add = (binaryfunc)add_LuaInstance_Wrapper,
	.nb_subtract = (binaryfunc)sub_LuaInstance_Wrapper,
	.nb_multiply = (binaryfunc)mul_LuaInstance_Wrapper,
	.nb_true_divide = (binaryfunc)div_LuaInstance_Wrapper,
	.nb_floor_divide = (binaryfunc)floordiv_LuaInstance_Wrapper,
	.nb_power = (ternaryfunc)pow_LuaInstance_Wrapper,
	.nb_remainder = (binaryfunc)mod_LuaInstance_Wrapper,
	.nb_negative = (unaryfunc)neg_LuaInstance_Wrapper,
	.nb_lshift = (binaryfunc)lshift_LuaInstance_Wrapper,
	.nb_rshift = (binaryfunc)rshift_LuaInstance_Wrapper,
	.nb_and = (binaryfunc)band_LuaInstance_Wrapper,
	.nb_or = (binaryfunc)bor_LuaInstance_Wrapper,
	.nb_xor = (binaryfunc)bxor_LuaInstance_Wrapper,
	.nb_invert = (unaryfunc)bnot_LuaInstance_Wrapper,
	.nb_matrix_multiply = (binaryfunc)concat_LuaInstance_Wrapper,
};


static PyMethodDef PyLua_Methods[3] = {
	{"LuaLoad",  &PyLua_LuaLoadModule, METH_VARARGS, "pylua.LuaLoad"},
	//{"LuaUnload",  &PyLua_LuaUnloadModule, METH_VARARGS, "pylua.LuaUnload"},
	{NULL, NULL, 0, NULL}
};


static struct PyModuleDef LUA_module = {
	PyModuleDef_HEAD_INIT,
	"pylua",
	"pylua doc",
	-1,
	PyLua_Methods
};


// Python Module Functions

static PyObject* PyLua_LuaLoadModule(PyObject* self, PyObject* args)
{
	PyObject* obj = PyObject_GetAttrString(pPylua_Module, "LuaModule");
	if (obj)
	{
		return PyObject_CallObject(obj, args);
	}

	PyErr_SetString(LuaError, "Unable to access pylua.LuaModule");
	return NULL;
}

static PyObject* PyLua_LuaUnloadModule(PyObject* self, PyObject* args)
{
	// TODO: implement
	Py_RETURN_NONE;
}

static int PyLua_LuaInit(PyLua_LuaModule* self, PyObject* args, PyObject* kwargs)
{
	char* name;
	if (!PyArg_ParseTuple(args, "s", &name))
	{
		PyErr_SetString(LuaError, "Got wrong arguments to LuaModule");
		return -1;
	}

	int load_status = luaL_loadfile(nL, name);

	switch (load_status)
	{
	case LUA_OK:
		if (lua_pcall(nL, 0, 0, 0) != LUA_OK)
		{
			PyErr_SetString(LuaError, "Error occured when executing the lua program");
			return -1;
		}
		break;

	case LUA_ERRSYNTAX:
		PyErr_SetString(LuaError, "Syntax Error in Lua File");
		return -1;

	case LUA_ERRMEM:
		PyErr_NoMemory();
		return -1;

	default:
		PyErr_SetString(LuaError, "Unknowen error occured when loading the file");
		return -1;
	}

	self->name = name;
	self->loaded = 1;

	return 0;
}

static PyObject* PyLua_LuaGet(PyLua_LuaModule* self, char* attr)
{
	lua_getglobal(nL, attr);
	PyObject* pReturn = PyLua_LuaToPython(nL, -1);
	lua_pop(nL, 1);
	return pReturn;
}

static int PyLua_LuaSet(PyLua_LuaModule* self, char* attr, PyObject* pValue)
{
	PyErr_SetString(LuaError, "Cannot Assign Values to LuaModule Objects");
	return -1;
}

static void PyLua_LuaGC(PyLua_LuaModule* self)
{
	// TODO: implement
	return;
}


// Lua function and thread wrapper functions

static PyObject* init_LuaFunc_Wrapper(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	if (kwargs)
	{
		// raise error
		PyErr_SetString(LuaError, PERR_KWARG_LUA);
		return NULL;
	}

	int index;
	int is_luathread;

	if (!PyArg_ParseTuple(args, "ii", &index, &is_luathread))
	{
		// TODO: change msg to "Expected x, y but got z"
		PyErr_SetString(LuaError, "Got wrong arguments to init_LuaFunc_Wrapper");
		return NULL;
	}

	self->index = index;
	self->is_luathread = is_luathread;
	self->thread_terminated = 0;

	return 0;
}

static PyObject* call_LuaFunc(PyLua_LuaFunc* self, PyObject* args, PyObject* kwargs)
{
	if (self->is_luathread)
	{
		// raise error
		PyErr_SetString(PyExc_TypeError, "Lua Thread Type is not Callable");
		return NULL;
	}

	if (kwargs)
	{
		// raise error
		PyErr_SetString(LuaError, PERR_KWARG_LUA);
		return NULL;
	}

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
		PyObject* pItem = PyTuple_GetItem(args, i);
		if (!pItem)
		{
			return raise_error("error should have occured");
		}
		if (PyLua_PythonToLua(cL, pItem) < 0)
		{
			return raise_error(PERR_CONVERT_PY_TO_LUA);
		}
	}

	// execute function
	if (lua_pcall(cL, arg_len, LUA_MULTRET, 0) != LUA_OK)
	{
		PyErr_Format(LuaError, ERR_CALL_LUA, lua_tostring(cL, -1));
		return NULL;
	}

	int return_len = lua_gettop(cL) - (stack_size + 1);

	PyObject* pReturn = create_return(cL, return_len);
	lua_pop(cL, return_len + 1);

	// check stack size
	CHECK_STACK_ZERO(cL);

	return pReturn;
}

static PyObject* iter_LuaCoroutine(PyLua_LuaFunc* self)
{
	if (!self->is_luathread)
	{
		// TODO: change msg to "expected lua thread but got x"
		PyErr_Format(PyExc_TypeError, "Not a Lua Thread");
		return NULL;
	}

	Py_INCREF(self);
	return (PyObject*)self;
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
		PyErr_Format(LuaError, ERR_CALL_LUA, lua_tostring(cL, -1));
		return NULL;
	}

	lua_pop(cL, 2);

	// check stack size
	CHECK_STACK_ZERO(cL);

	return pReturn;
}

static void gc_LuaFunc(PyLua_LuaFunc* self)
{
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	luaL_unref(cL, -1, self->index);
	lua_pop(cL, 1);

	CHECK_STACK_ZERO(cL);
}


// Lua table wrapper functions

static int init_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* args, PyObject* kwargs)
{
	if (kwargs)
	{
		// raise error
		PyErr_SetString(LuaError, PERR_KWARG_LUA);
		return -1;
	}

	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
	{
		PyErr_SetString(LuaError, "Got wrong arguments to init_LuaFunc_Wrapper");
		return -1;
	}

	self->index = index;

	return 0;
}

static PyObject* getelem_LuaTable_Wrapper(PyLua_LuaTable* self, PyObject* pKey)
{
	PyObject* pReturn = NULL;

	// to string
	PyObject* pStr = PyObject_Str(pKey);
	if (!pStr)
	{
		return raise_error("error should have occured");
	}

	PyObject* encodedString = PyUnicode_AsEncodedString(pKey, "UTF-8", "strict");
	const char* key_str = NULL;

	// string
	if (encodedString)
	{
		key_str = PyBytes_AsString(encodedString);
		if (!key_str)
		{
			return raise_error("error should have occured: memroy error");
		}
	}
	else
	{
		return raise_error("error should have occured: memroy error");
	}

	// get table
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	lua_rawgeti(cL, -2, self->index);

	// get item
	lua_getfield(cL, -1, key_str);
	pReturn = PyLua_LuaToPython(cL, -1);
	if (!pReturn)
	{
		return raise_error(PERR_CONVERT_LUA_TO_PY);
	}

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
	if (!pStr)
	{
		raise_error("error should have occured");
		return -1;
	}

	PyObject* encodedString = PyUnicode_AsEncodedString(pKey, "UTF-8", "strict");
	const char* key_str = NULL;

	// string
	if (encodedString)
	{
		key_str = PyBytes_AsString(encodedString);
		if (!key_str)
		{
			raise_error("error should have occured: memroy error");
			return -1;
		}
	}
	else
	{
		raise_error("error should have occured: memroy error");
		return -1;
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

	if (PyLua_PythonToLua(cL, pValue) < 0)
	{
		raise_error(PERR_CONVERT_PY_TO_LUA);

		Py_DECREF(pStr);
		Py_DECREF(encodedString);
		return -1;
	}

	lua_setfield(cL, -2, key_str);

	lua_pop(cL, 2);

	Py_DECREF(pStr);
	Py_DECREF(encodedString);
	
	CHECK_STACK_ZERO(cL);
	
	return 0;
}

static PyObject* iter_LuaTable(PyLua_LuaTable* self)
{
	// to be implemented
	Py_RETURN_NONE;
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
	// TODO: use optional kwargs if initializer function is not called "new"
	lua_getfield(cL, -1, "new");
	lua_replace(cL, stack_size + 2);


	// python args to lua
	Py_ssize_t arg_len = PyTuple_Size(args);

	for (int i = 0; i < arg_len; i++)
	{
		if (PyLua_PythonToLua(cL, PyTuple_GetItem(args, i)) < 0)
		{
			return raise_error(PERR_CONVERT_PY_TO_LUA);
		}
	}

	// call function
	if (lua_pcall(cL, arg_len + 1, 1, 0) != LUA_OK)
	{
		PyErr_Format(LuaError, ERR_CALL_PY, lua_tostring(cL, -1));
		return NULL;
	}

	pReturn = PyLua_LuaToPython(cL, -1);
	if (!pReturn)
	{
		return raise_error(PERR_CONVERT_LUA_TO_PY);
	}
	lua_pop(cL, 2); // result and registry

	CHECK_STACK_ZERO(cL);

	return pReturn;
}

static void gc_LuaTable(PyLua_LuaTable* self)
{
	lua_pushvalue(cL, LUA_REGISTRYINDEX);
	luaL_unref(cL, -1, self->index);
	lua_pop(cL, 1);

	CHECK_STACK_ZERO(cL);
}


// Lua instance (table) wrapper functions

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
		if (!pReturn)
		{
			return raise_error(PERR_CONVERT_LUA_TO_PY);
		}

		lua_pop(cL, 3);
	}
	// else wrap it around wrapper_around_method function and set closures values to be.
	else 
	{
		lua_pushcclosure(cL, wrapper_around_method, 2);
		pReturn = PyLua_LuaToPython(cL, -1);
		if (!pReturn)
		{
			return raise_error(PERR_CONVERT_LUA_TO_PY);
		}
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

	if (PyLua_PythonToLua(cL, pValue) < 0)
	{
		raise_error(PERR_CONVERT_LUA_TO_PY);
		return -1;
	}

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
		PyErr_SetString(LuaError, PERR_KWARG_LUA);
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
		PyObject* pItem = PyTuple_GetItem(args, i);
		if (!pItem)
		{
			return raise_error("error should have occured");
		}

		if (PyLua_PythonToLua(cL, pItem) < 0)
		{
			return raise_error(PERR_CONVERT_PY_TO_LUA);
		}

	}

	// call function
	if (lua_pcall(cL, arg_len + 1, 1, 0) != LUA_OK)
	{
		PyErr_Format(LuaError, ERR_CALL_LUA, lua_tostring(cL, -1));
		return NULL;
	}

	pReturn = PyLua_LuaToPython(cL, -1);
	if (!pReturn)
	{
		return raise_error(PERR_CONVERT_LUA_TO_PY);
	}

	lua_pop(cL, 2); // result and registry

	CHECK_STACK_ZERO(cL);

	return pReturn;
}

static PyObject* string_LuaInstance_Wrapper(PyLua_LuaTable* self)
{
	return operation_LuaTable_base(self, Py_None, "__tostring");
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

static PyObject* pow_LuaInstance_Wrapper(PyLua_LuaTable* self, PyObject* other, PyObject* t)
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

static PyObject* neg_LuaInstance_Wrapper(PyLua_LuaTable* self)
{
	return operation_LuaTable_base(self, Py_None, "__unm");
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
