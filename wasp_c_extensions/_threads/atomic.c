// wasp_c_extensions/_threads/atomic.c
//
//Copyright (C) 2016 the wasp-c-extensions authors and contributors
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

PyObject* __py_int_add_fn__ = 0;

static PyObject* WAtomicCounter_Type_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
static void WAtomicCounter_Type_dealloc(WAtomicCounter_Object* self);
static int WAtomicCounter_Object_init(WAtomicCounter_Object *self, PyObject *args, PyObject *kwargs);
static PyObject* WAtomicCounter_Object___int__(WAtomicCounter_Object* self, PyObject *args);
static PyObject* WAtomicCounter_Object_increase_counter(WAtomicCounter_Object* self, PyObject* args);

static PyMethodDef WAtomicCounter_Type_methods[] = {
	{
		"__int__", (PyCFunction) WAtomicCounter_Object___int__, METH_NOARGS,
		"Return reference to integer object that is used for internal counter value"
	},
	{
		"increase_counter", (PyCFunction) WAtomicCounter_Object_increase_counter, METH_VARARGS,
		"Increase current counter value and return a result\n"
		"\n"
		":param value: increment with which counter value should be increased (may be negative)\n"
		":return: int"
	},

	{NULL}
};

PyTypeObject WAtomicCounter_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = __STR_PACKAGE_NAME__"."__STR_MODULE_NAME__"."__STR_ATOMIC_COUNTER_NAME__,
	.tp_doc = "Counter with atomic increase operation",
	.tp_basicsize = sizeof(WAtomicCounter_Type),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_new = WAtomicCounter_Type_new,
	.tp_init = (initproc) WAtomicCounter_Object_init,
	.tp_dealloc = (destructor) WAtomicCounter_Type_dealloc,
	.tp_methods = WAtomicCounter_Type_methods,
	.tp_weaklistoffset = offsetof(WAtomicCounter_Object, weakreflist)
};

static PyObject* WAtomicCounter_Type_new(PyTypeObject* type, PyObject* args, PyObject* kwargs) {

	__WASP_DEBUG_PRINTF__("Allocation of \""__STR_ATOMIC_COUNTER_NAME__"\" object");

	WAtomicCounter_Object* self;
	self = (WAtomicCounter_Object *) type->tp_alloc(type, 0);

	if (self != NULL) {
		self->__int_value = (PyLongObject*) PyLong_FromLong(0);
		if (self->__int_value == NULL) {
			Py_DECREF(self);
			return NULL;
		}
	}

	__WASP_DEBUG_PRINTF__("Object \""__STR_ATOMIC_COUNTER_NAME__"\" was allocated");

	return (PyObject *) self;
}

static int WAtomicCounter_Object_init(WAtomicCounter_Object *self, PyObject *args, PyObject *kwargs) {

	__WASP_DEBUG_PRINTF__("Initialization of \""__STR_ATOMIC_COUNTER_NAME__"\" object");

	static char *kwlist[] = {"value", NULL};
	PyObject* value = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O!", kwlist,  &PyLong_Type, &value)) {
		return -1;
	}

	if (value) {
		Py_DECREF(self->__int_value);
		self->__int_value = (PyLongObject*) value;
		Py_INCREF(self->__int_value);
	}

	__WASP_DEBUG_PRINTF__("Object \""__STR_ATOMIC_COUNTER_NAME__"\" was initialized");

	return 0;
}

static void WAtomicCounter_Type_dealloc(WAtomicCounter_Object* self) {

	__WASP_DEBUG_PRINTF__("Deallocation of \""__STR_ATOMIC_COUNTER_NAME__"\" object");

	if (self->weakreflist != NULL)
        	PyObject_ClearWeakRefs((PyObject *) self);

	Py_DECREF(self->__int_value);
	Py_TYPE(self)->tp_free((PyObject *) self);

	__WASP_DEBUG_PRINTF__("Object \""__STR_ATOMIC_COUNTER_NAME__"\" was deallocated");
}

static PyObject* WAtomicCounter_Object___int__(WAtomicCounter_Object* self, PyObject *args) {

	__WASP_DEBUG_PRINTF__("A call to \""__STR_ATOMIC_COUNTER_NAME__".__int__\" method was made");

	Py_INCREF(self->__int_value);
	return (PyObject*) self->__int_value;
}

static PyObject* WAtomicCounter_Object_increase_counter(WAtomicCounter_Object* self, PyObject* args)
{
	__WASP_DEBUG_PRINTF__("A call to \""__STR_ATOMIC_COUNTER_NAME__".increase_counter\" method was made");

	PyObject* increment;
	if (!PyArg_ParseTuple(args, "O!", &PyLong_Type, &increment)){
		return NULL;
	}
	Py_INCREF(increment);

	PyObject* increase_fn_args = PyTuple_Pack(2, self->__int_value, increment);
	if (increase_fn_args == NULL){
		return NULL;
	}

	PyObject* increment_result = PyObject_CallObject(__py_int_add_fn__, increase_fn_args);
	Py_DECREF(increment);
	Py_DECREF(increase_fn_args);

	if (increment_result == NULL){
		return NULL;
	}
	Py_DECREF(self->__int_value);
	self->__int_value = (PyLongObject*) increment_result;
	Py_INCREF(self->__int_value);

	return (PyObject*) self->__int_value;
}
