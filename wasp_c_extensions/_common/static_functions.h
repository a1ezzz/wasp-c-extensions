// wasp_c_extensions/_common/static_functions.h
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

#ifndef __WASP_C_EXTENSIONS__COMMON_STATIC_FUNCTIONS_H__
#define __WASP_C_EXTENSIONS__COMMON_STATIC_FUNCTIONS_H__

static inline PyObject* __c_integer_operator(
	PyLongObject* py_long_value, const char* method, long method_argument, const char* err_msg
) {
	PyObject* result = PyObject_CallMethod((PyObject *) py_long_value, method, "(i)", method_argument);
	if (result == NULL){
		if (err_msg != NULL){
			PyErr_SetString(PyExc_RuntimeError, err_msg);
		}
		return NULL;
	}
	return result;
}

static inline PyObject* __py_integer_operator(
	PyLongObject* py_long_value, const char* method, PyObject* method_argument, const char* err_msg
) {
	PyObject* result = PyObject_CallMethod((PyObject *) py_long_value, method, "(O)", method_argument);
	if (result == NULL){
		if (err_msg != NULL){
			PyErr_SetString(PyExc_RuntimeError, err_msg);
		}
		return NULL;
	}
	return result;
}

static inline int __reassign_with_c_integer_operator(
	PyLongObject** py_long_value, const char* method, long method_argument, const char* err_msg
) {
	PyObject* next_value = PyObject_CallMethod((PyObject *) *py_long_value, method, "(i)", method_argument);
	if (next_value == NULL){
		if (err_msg != NULL){
			PyErr_SetString(PyExc_RuntimeError, err_msg);
		}
		return -1;
	}

	Py_DECREF(*py_long_value);
	(*py_long_value) = (PyLongObject*) next_value;
	return 0;
}

static inline int __comparision_c_integer_operator(
	PyLongObject* py_long_value, const char* method, long method_argument, const char* call_err_msg,
	const char* is_true_err_msg
) {
	int is_true = 0;
	PyObject* comparision_result = PyObject_CallMethod((PyObject *) py_long_value, method, "(i)", method_argument);
	if (comparision_result == NULL){
		PyErr_SetString(PyExc_RuntimeError, call_err_msg);
		return -1;
	}

	is_true = PyObject_IsTrue(comparision_result);
	Py_DECREF(comparision_result);

	if (is_true == -1){
		PyErr_SetString(PyExc_RuntimeError, is_true_err_msg);
	}

	return is_true;
}

#endif // __WASP_C_EXTENSIONS__COMMON_STATIC_FUNCTIONS_H__
