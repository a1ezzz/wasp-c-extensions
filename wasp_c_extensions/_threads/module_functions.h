// wasp_c_extensions/_threads/module_functions.h
//
//Copyright (C) 2018 the wasp-c-extensions authors and contributors
//<see AUTHORS file>
//
//This file is part of wasp-c-extensions.
//
//Wasp-c-extensions is free software: you can redistribute it and/or modify
//it under the terms of the GNU Lesser General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//Wasp-c-extensions is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public License
//along with wasp-c-extensions.  If not, see <http://www.gnu.org/licenses/>.

static PyObject* awareness_wait(PyObject *self, PyObject *args, PyObject *kwargs)
{
	__WASP_DEBUG_FN_CALL__;

	PyObject* event = NULL;
	PyObject* sync_fn = NULL;
	PyObject* timeout = NULL;
	PyObject* fn_result = NULL;
	PyObject* fn_args = NULL;

	static char *args_list[] = {"event", "sync_fn", "timeout", NULL};
	int is_true = 0;

	if (! PyArg_ParseTupleAndKeywords(args, kwargs, "OO|O", args_list, &event, &sync_fn, &timeout)){
		return NULL;
	}

	Py_INCREF(sync_fn);

	if (PyCallable_Check(sync_fn) != 1){
		PyErr_SetString(PyExc_ValueError, "A 'sync_fn' variable must be a 'callable' object");
		Py_DECREF(sync_fn);
		return NULL;
	}

	fn_args = PyTuple_Pack(0);
	fn_result = PyObject_Call(sync_fn, fn_args, NULL);
	Py_DECREF(sync_fn);
	Py_DECREF(fn_args);

	if (fn_result == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "A 'sync_fn' call error!");
		return NULL;
	}

	is_true = PyObject_IsTrue(fn_result);
	Py_DECREF(fn_result);

	if (is_true == -1) {
		PyErr_SetString(PyExc_RuntimeError, "A 'sync_fn' comparision error!");
		return NULL;
	}

	Py_INCREF(event);
	if (is_true == 1) {
		PyObject_CallMethod(event, "set", NULL);
		Py_DECREF(event);
		Py_RETURN_TRUE;
	}

	PyObject_CallMethod(event, "clear", NULL);

	if (timeout != NULL && timeout != Py_None) {
		Py_INCREF(timeout);
		fn_result = PyObject_CallMethod(event, "wait", "O", timeout);
		Py_DECREF(timeout);
	}
	else {
		fn_result = PyObject_CallMethod(event, "wait", NULL);
	}

	Py_DECREF(event);
	return fn_result;
}
