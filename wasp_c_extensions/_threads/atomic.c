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
#include "static_functions.h"

static PyObject* WAtomicCounter_Type_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
static void WAtomicCounter_Type_dealloc(WAtomicCounter_Object* self);
static int WAtomicCounter_Object_init(WAtomicCounter_Object *self, PyObject *args, PyObject *kwargs);
static PyObject* WAtomicCounter_Object___int__(PyObject* self);
static PyObject* WAtomicCounter_Object_increase_counter(WAtomicCounter_Object* self, PyObject* args);
static PyObject* WAtomicCounter_Object_set(WAtomicCounter_Object* self, PyObject* args);
static PyObject* WAtomicCounter_Object_test_and_set(WAtomicCounter_Object* self, PyObject* args);
static PyObject* WAtomicCounter_Object_compare_and_set(WAtomicCounter_Object* self, PyObject* args);
static int WAtomicCounter_Object_valid_value(PyLongObject* int_value, bool negative);

PyLongObject* __zero = NULL;

static PyMethodDef WAtomicCounter_Type_methods[] = {
	{
		"increase_counter", (PyCFunction) WAtomicCounter_Object_increase_counter, METH_VARARGS,
		"Increase current counter value and return a result\n"
		"\n"
		":param value: increment with which counter value should be increased (may be negative)\n"
		":type value: int\n"
		"\n"
		":raise ValueError: if a counter is non-negative and increment will make it negative\n"
		"\n"
		":rtype: int"
	},
	{
		"set", (PyCFunction) WAtomicCounter_Object_set, METH_VARARGS,
		"Set a new value and return a previous one. A new value must be non-negative if this counter was created as\n"
		"non-negative\n"
		"\n"
		":param value: new value to set\n"
		":type value: int\n"
		"\n"
		":raise ValueError: if a negative value was requested for a non-negative counter\n"
		"\n"
		":rtype: int"
	},
	{
		"test_and_set", (PyCFunction) WAtomicCounter_Object_test_and_set, METH_VARARGS,
		"Compare current value with a new one. If a comparision result is True - set a new value, and return old\n"
		"value. None is return if a comparision result is False\n"
		"\n"
		":param operation: comparision operation. One of:\n"
		"\t"WASP_ATOMIC_LT_TEST_NAME" - <\n"
		"\t"WASP_ATOMIC_LE_TEST_NAME" - <=\n"
		"\t"WASP_ATOMIC_EQ_TEST_NAME" - == (the most useless one =) )\n"
		"\t"WASP_ATOMIC_NE_TEST_NAME" - !=\n"
		"\t"WASP_ATOMIC_GT_TEST_NAME" - >\n"
		"\t"WASP_ATOMIC_GE_TEST_NAME" - >=\n"
		"\n"
		":param value: counter to test and to set\n"
		":type value: "__STR_ATOMIC_COUNTER_NAME__"\n"
		"\n"
		":raise ValueError: if a negative value was requested to test for a non-negative counter\n"
		"\n"
		":rtype: None or int\n"
	},
	{
		"compare_and_set", (PyCFunction) WAtomicCounter_Object_compare_and_set, METH_VARARGS,
		"Compare current value with a test one. If a test value is the same as this counter holds - set a new value,\n"
		"and return old value. None is return if a test value differs\n"
		"\n"
		":param test_value: counter to test\n"
		":type test_value: "__STR_ATOMIC_COUNTER_NAME__"\n"
		"\n"
		":param set_value: counter to set\n"
		":type set_value: "__STR_ATOMIC_COUNTER_NAME__"\n"
		"\n"
		":raise ValueError: if a negative value was requested to set for a non-negative counter\n"
		"\n"
		":rtype: None or int\n"
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

    Py_INCREF(__zero);
	self->__int_value = __zero;
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
		result = WAtomicCounter_Object_valid_value(value, self->__negative);
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
	if (WAtomicCounter_Object_valid_value(increment_result, self->__negative) != 0) {
		Py_DECREF(increment_result);
		return NULL;
	}

	Py_DECREF(self->__int_value);
	self->__int_value = (PyLongObject*) increment_result;
	return (PyObject*) self->__int_value;
}

static int WAtomicCounter_Object_valid_value(PyLongObject* int_value, bool negative) {
	__WASP_DEBUG_FN_CALL__;

	int result = 0;

    if (! negative) {
        result = __long_rich_compare_bool(int_value, __zero, Py_LT);
        if (result == - 1){
            PyErr_SetString(PyExc_RuntimeError, "Unable to check zero comparision result");
        }
        else if (result == 1){
            PyErr_SetString(PyExc_ValueError, "This counter can not be negative");
            __WASP_DEBUG_PRINTF__("The spotted value is invalid for the counter");
        }
    }

    return result;
}

static PyObject* WAtomicCounter_Object_set(WAtomicCounter_Object* self, PyObject* args) {
    PyLongObject* new_value = NULL;
    PyLongObject* previous_value = NULL;

	if (! PyArg_ParseTuple(args, "O!", &PyLong_Type, &new_value)){
		return NULL;
	}

	Py_INCREF(new_value);
	if (WAtomicCounter_Object_valid_value(new_value, self->__negative) != 0) {
		Py_DECREF(new_value);
		return NULL;
	}

    previous_value = self->__int_value;
	self->__int_value = new_value;
    return (PyObject*) previous_value;
}

static PyObject* WAtomicCounter_Object_test_and_set(WAtomicCounter_Object* self, PyObject* args) {

    WAtomicCounter_Object* test_counter = NULL;
    PyLongObject* previous_value = NULL;
    int test_operation = -1;
    int is_true = -1;

	if (! PyArg_ParseTuple(args, "iO!", &test_operation, &WAtomicCounter_Type, &test_counter)){
		return NULL;
	}

    Py_INCREF(test_counter);

	if (WAtomicCounter_Object_valid_value(test_counter->__int_value, self->__negative) != 0) {
		Py_DECREF(test_counter);
		return NULL;
	}

    is_true = __long_rich_compare_bool(self->__int_value, test_counter->__int_value, test_operation);
	if (is_true == -1){
		PyErr_SetString(PyExc_RuntimeError, "Unable to check comparision result");
	    return NULL;
	}
	else if (is_true == 0){
	    Py_RETURN_NONE;
	}

    previous_value = self->__int_value;
	self->__int_value = test_counter->__int_value;
    Py_INCREF(test_counter->__int_value);
    Py_DECREF(test_counter);
    return (PyObject*) previous_value;
}

static PyObject* WAtomicCounter_Object_compare_and_set(WAtomicCounter_Object* self, PyObject* args) {
    WAtomicCounter_Object* test_counter = NULL;
    WAtomicCounter_Object* new_counter = NULL;
    PyLongObject* previous_value = NULL;
    int is_true = -1;

	if (! PyArg_ParseTuple(args, "O!O!", &WAtomicCounter_Type, &test_counter, &WAtomicCounter_Type, &new_counter)){
		return NULL;
	}

    Py_INCREF(new_counter);
	if (WAtomicCounter_Object_valid_value(new_counter->__int_value, self->__negative) != 0) {
		Py_DECREF(new_counter);
		return NULL;
	}

    Py_INCREF(test_counter);
    is_true = __long_rich_compare_bool(self->__int_value, test_counter->__int_value, Py_EQ);
    Py_DECREF(test_counter);
	if (is_true == -1){
        Py_DECREF(new_counter);
		PyErr_SetString(PyExc_RuntimeError, "Unable to check comparision result");
	    return NULL;
	}
	else if (is_true == 0){
        Py_DECREF(new_counter);
	    Py_RETURN_NONE;
	}

    previous_value = self->__int_value;
	self->__int_value = new_counter->__int_value;
    Py_INCREF(new_counter->__int_value);
    Py_DECREF(new_counter);
    return (PyObject*) previous_value;
}