// wasp_c_extensions/_threads/atomic.c
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

#include "atomic.h"

static PyObject* WAtomicCounter_Type_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
static void WAtomicCounter_Type_dealloc(WAtomicCounter_Object* self);
static int WAtomicCounter_Object_init(WAtomicCounter_Object *self, PyObject *args, PyObject *kwargs);
static PyObject* WAtomicCounter_Object___int__(PyObject* self);
static PyObject* WAtomicCounter_Object_increase_counter(WAtomicCounter_Object* self, PyObject* args);
static int WAtomicCounter_Object_valid_value(WAtomicCounter_Object* self, PyLongObject* int_value);

static PyMethodDef WAtomicCounter_Type_methods[] = {
	{
		"increase_counter", (PyCFunction) WAtomicCounter_Object_increase_counter, METH_VARARGS,
		"Increase current counter value and return a result\n"
		"\n"
		":param value: increment with which counter value should be increased (may be negative)\n"
		":return: int"
	},

	{NULL}
};

static PyNumberMethods WAtomicCounter_Type_as_number = {
    .nb_int = WAtomicCounter_Object___int__
};

PyTypeObject WAtomicCounter_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = __STR_PACKAGE_NAME__"."__STR_THREADS_MODULE_NAME__"."__STR_ATOMIC_COUNTER_NAME__,
	.tp_doc = "Counter with atomic increase operation",
	.tp_basicsize = sizeof(WAtomicCounter_Type),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_new = WAtomicCounter_Type_new,
	.tp_init = (initproc) WAtomicCounter_Object_init,
	.tp_dealloc = (destructor) WAtomicCounter_Type_dealloc,
	.tp_methods = WAtomicCounter_Type_methods,
	.tp_as_number = &WAtomicCounter_Type_as_number,
	.tp_weaklistoffset = offsetof(WAtomicCounter_Object, __weakreflist)
};

static PyObject* WAtomicCounter_Type_new(PyTypeObject* type, PyObject* args, PyObject* kwargs) {

	__WASP_DEBUG_PRINTF__("Allocation of \""__STR_ATOMIC_COUNTER_NAME__"\" object");

	WAtomicCounter_Object* self = NULL;
	self = (WAtomicCounter_Object *) type->tp_alloc(type, 0);

	if (self == NULL){
		return PyErr_NoMemory();
	}

	self->__int_value = (PyLongObject*) PyLong_FromLong(0);  // NOTE: returns new reference

	if (self->__int_value == NULL) {
		Py_DECREF(self);
		return PyErr_NoMemory();
	}
	self->__zero = self->__int_value;
	Py_INCREF(self->__zero);
	self->__negative = true;

	__WASP_DEBUG_PRINTF__("Object \""__STR_ATOMIC_COUNTER_NAME__"\" was allocated");

	return (PyObject *) self;
}

static int WAtomicCounter_Object_init(WAtomicCounter_Object *self, PyObject *args, PyObject *kwargs) {

	__WASP_DEBUG_PRINTF__("Initialization of \""__STR_ATOMIC_COUNTER_NAME__"\" object");

	static char *kwlist[] = {"value", "negative", NULL};
	PyLongObject* value = NULL;
	int result = 0;

	if (! PyArg_ParseTupleAndKeywords(args, kwargs, "|O!p", kwlist, &PyLong_Type, &value, &self->__negative)) {
		return -1;
	}

	if (value != NULL) {

		Py_INCREF(value);  // NOTE: values that were parsed as "O" do not increment ref. counter
		result = WAtomicCounter_Object_valid_value(self, value);
		if (result != 0) {
			Py_DECREF(value);
			return -1;
		}

		Py_DECREF(self->__int_value);  // NOTE: we no longer need old value
		self->__int_value = (PyLongObject*) value;
	}

	__WASP_DEBUG_PRINTF__("Object \""__STR_ATOMIC_COUNTER_NAME__"\" was initialized");

	return 0;
}

static void WAtomicCounter_Type_dealloc(WAtomicCounter_Object* self) {

	__WASP_DEBUG_PRINTF__("Deallocation of \""__STR_ATOMIC_COUNTER_NAME__"\" object");

	if (self->__weakreflist != NULL)
        	PyObject_ClearWeakRefs((PyObject *) self);

	Py_XDECREF(self->__int_value);  // NOTE: value must be destroyed
	Py_XDECREF(self->__zero);  // NOTE: value must be destroyed
	Py_TYPE(self)->tp_free((PyObject *) self);

	__WASP_DEBUG_PRINTF__("Object \""__STR_ATOMIC_COUNTER_NAME__"\" was deallocated");
}

static PyObject* WAtomicCounter_Object___int__(PyObject* self) {

	__WASP_DEBUG_PRINTF__("A call to \""__STR_ATOMIC_COUNTER_NAME__".__int__\" method was made");

	WAtomicCounter_Object* counter = (WAtomicCounter_Object*) self;
	Py_INCREF(counter->__int_value);  // NOTE: increasing since this value is returned from C-function
	return (PyObject*) counter->__int_value;
}

static PyObject* WAtomicCounter_Object_increase_counter(WAtomicCounter_Object* self, PyObject* args)
{
	__WASP_DEBUG_PRINTF__("A call to \""__STR_ATOMIC_COUNTER_NAME__".increase_counter\" method was made");

	PyLongObject* increment = NULL;
	PyLongObject* increment_result = NULL;

	if (! PyArg_ParseTuple(args, "O!", &PyLong_Type, &increment)){
		return NULL;
	}
	Py_INCREF(increment);  // NOTE: pointers from "O" must be counted (as long as we pass it to python function)
	increment_result = (PyLongObject*) PyLong_Type.tp_as_number->nb_add(
	    (PyObject*) self->__int_value, (PyObject*) increment
	);
	Py_DECREF(increment);  // NOTE: there is no need in python function arguments

	if (increment_result == NULL){
		PyErr_SetString(PyExc_RuntimeError, "Unable to calculate a result");
		return NULL;
	}

	Py_INCREF(increment_result);
	if (WAtomicCounter_Object_valid_value(self, increment_result) != 0) {
		Py_DECREF(increment_result);
		return NULL;
	}

	Py_DECREF(self->__int_value);
	self->__int_value = (PyLongObject*) increment_result;
	return (PyObject*) self->__int_value;
}

static int WAtomicCounter_Object_valid_value(WAtomicCounter_Object* self, PyLongObject* int_value) {
	__WASP_DEBUG_FN_CALL__;

	int is_true = 0;

	if (! self->__negative){

	    is_true = PyObject_RichCompareBool((PyObject*) int_value, (PyObject*) self->__zero, Py_LT);

		if (is_true == 1){
			__WASP_DEBUG_PRINTF__("The spotted value is invalid for the counter");
			PyErr_SetString(PyExc_ValueError, "This counter instance can not be negative");
		}
	}

	return is_true;
}
