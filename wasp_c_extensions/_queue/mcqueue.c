// wasp_c_extensions/_queue/mcqueue.c
//
//Copyright (C) 2019 the wasp-c-extensions authors and contributors
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

#include "mcqueue.h"
#include "_common/static_functions.h"

static PyObject* WMultipleConsumersQueue_Type_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
static void WMultipleConsumersQueue_Type_dealloc(WMultipleConsumersQueue_Object* self);
static int WMultipleConsumersQueue_Object_init(WMultipleConsumersQueue_Object *self, PyObject *args, PyObject *kwargs);

static PyObject* WMultipleConsumersQueue_Object_subscribe(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_unsubscribe(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_push(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_pop(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_has(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_msg_index(WMultipleConsumersQueue_Object* self, PyObject* msg_index);
static int WMultipleConsumersQueue_Object_has_raw(WMultipleConsumersQueue_Object* self, PyObject* msg_index);
static PyObject* WMultipleConsumersQueue_Object_count(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_clean(
	WMultipleConsumersQueue_Object* self, Py_ssize_t from_el, Py_ssize_t el_count
);

static PyMethodDef WMultipleConsumersQueue_Type_methods[] = {
	{
		"subscribe", (PyCFunction) WMultipleConsumersQueue_Object_subscribe, METH_NOARGS,
		"" // TODO: update docs!
	},
	{
		"unsubscribe", (PyCFunction) WMultipleConsumersQueue_Object_unsubscribe, METH_VARARGS,
		"" // TODO: update docs!
	},
	{
		"push", (PyCFunction) WMultipleConsumersQueue_Object_push, METH_VARARGS,
		"" // TODO: update docs!
	},
	{
		"pop", (PyCFunction) WMultipleConsumersQueue_Object_pop, METH_VARARGS,
		"" // TODO: update docs!
	},
	{
		"has", (PyCFunction) WMultipleConsumersQueue_Object_has, METH_VARARGS,
		"" // TODO: update docs!
	},
	{
		"count", (PyCFunction) WMultipleConsumersQueue_Object_count, METH_NOARGS,
		"" // TODO: update docs!
	},
	{NULL}
};

PyTypeObject WMultipleConsumersQueue_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = __STR_PACKAGE_NAME__"."__STR_QUEUE_MODULE_NAME__"."__STR_MCQUEUE_NAME__,
	.tp_doc = "This is a simple queue that allows multiple consumers get their own copy of incoming data",
	.tp_basicsize = sizeof(WMultipleConsumersQueue_Type),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_new = WMultipleConsumersQueue_Type_new,
	.tp_init = (initproc) WMultipleConsumersQueue_Object_init,
	.tp_dealloc = (destructor) WMultipleConsumersQueue_Type_dealloc,
	.tp_methods = WMultipleConsumersQueue_Type_methods,
	.tp_weaklistoffset = offsetof(WMultipleConsumersQueue_Object, __weakreflist)
};

static PyObject* WMultipleConsumersQueue_Type_new(PyTypeObject* type, PyObject* args, PyObject* kwargs) {

	__WASP_DEBUG_PRINTF__("Allocation of \""__STR_MCQUEUE_NAME__"\" object");

	WMultipleConsumersQueue_Object* self;
	self = (WMultipleConsumersQueue_Object *) type->tp_alloc(type, 0);
	if (self == NULL) {
		return PyErr_NoMemory();
	}

	self->__callback = NULL;
	self->__index_delta = NULL;
	self->__subscribers = NULL;
	self->__queue = NULL;

	self->__index_delta = (PyLongObject*) PyLong_FromLong(0);
	if (self->__index_delta == NULL){
		Py_DECREF(self);
		return PyErr_NoMemory();
	}

	self->__subscribers = (PyLongObject*) PyLong_FromLong(0);
	if (self->__subscribers == NULL){
		Py_DECREF(self->__index_delta);
		Py_DECREF(self);
		return PyErr_NoMemory();
	}

	self->__queue = (PyListObject*) PyList_New(0);
	if (self->__queue == NULL){
		Py_DECREF(self->__subscribers);
		Py_DECREF(self->__index_delta);
		Py_DECREF(self);
		return PyErr_NoMemory();
	}

	__WASP_DEBUG_PRINTF__("Object \""__STR_MCQUEUE_NAME__"\" was allocated");

	return (PyObject *) self;
}

static int WMultipleConsumersQueue_Object_init(WMultipleConsumersQueue_Object *self, PyObject *args, PyObject *kwargs) {

	__WASP_DEBUG_FN_CALL__;

	static char *kwlist[] = {"callback", NULL};
	PyObject* callback = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist,  &callback)) {
		return -1;
	}

	if (callback != NULL){
		Py_INCREF(callback);  // NOTE: 'O'-object must increase theirs counter manually
		if (PyCallable_Check(callback) != 1){
			PyErr_SetString(PyExc_ValueError, "A callback variable must be 'callable' object");
			Py_DECREF(callback);
			return 0;
		}
		self->__callback = callback;
	}

	__WASP_DEBUG_PRINTF__("Object \""__STR_MCQUEUE_NAME__"\" was initialized");

	return 0;
}


static void WMultipleConsumersQueue_Type_dealloc(WMultipleConsumersQueue_Object* self) {

	__WASP_DEBUG_PRINTF__("Deallocation of \""__STR_MCQUEUE_NAME__"\" object");

	if (self->__weakreflist != NULL) {
        	PyObject_ClearWeakRefs((PyObject *) self);
        }

	if (self->__queue != NULL){
		WMultipleConsumersQueue_Object_clean(self, 0, PyList_Size((PyObject *) self->__queue));
		Py_DECREF(self->__queue);
	}

	if (self->__subscribers != NULL){
		Py_DECREF(self->__subscribers);
	}

	if (self->__index_delta != NULL){
		Py_DECREF(self->__index_delta);
	}

	if (self->__callback != NULL) {
		Py_DECREF(self->__callback);
	}

	Py_TYPE(self)->tp_free((PyObject *) self);

	__WASP_DEBUG_PRINTF__("Object \""__STR_MCQUEUE_NAME__"\" was deallocated");
}

static PyObject* WMultipleConsumersQueue_Object_subscribe(WMultipleConsumersQueue_Object* self, PyObject* args){
	__WASP_DEBUG_FN_CALL__;

	PyObject* next_message = NULL;

	next_message = __c_integer_operator(
		self->__index_delta, "__radd__", PyList_Size((PyObject *) self->__queue),
		"Unable to find next message index"
	);
	if (next_message != NULL){
		if (__reassign_with_c_integer_operator(
			&self->__subscribers, "__radd__", 1, "Unable to increase number of subscribers"
		) == 0){
			return next_message;
		}
		Py_DECREF(next_message);
	}

	return NULL;
}

static PyObject* WMultipleConsumersQueue_Object_unsubscribe(WMultipleConsumersQueue_Object* self, PyObject* args) {
	__WASP_DEBUG_FN_CALL__;

	PyObject* msg = NULL;
	PyObject* sub_counter = NULL;
	PyObject* packed_msg = NULL;
	PyObject* py_msg_index = NULL;
	int is_true = 0;
	Py_ssize_t c_msg_index = 0;
	Py_ssize_t i = 0;
	Py_ssize_t drop_till = 0;
	Py_ssize_t queue_length = PyList_Size((PyObject *) self->__queue);

	py_msg_index = WMultipleConsumersQueue_Object_msg_index(self, args);
	if (py_msg_index == NULL){
		return NULL;
	}

	c_msg_index = PyLong_AsSsize_t(py_msg_index);
	Py_DECREF(py_msg_index);
	if (PyErr_Occurred() != NULL) {
		return NULL;
	}

	if (__reassign_with_c_integer_operator(
		&self->__subscribers, "__sub__", 1, "Unable to decrease number of subscribers"
	) != 0){
		return NULL;
	}

	for (i = c_msg_index; i < queue_length; i++) {
		packed_msg = PyList_GetItem((PyObject*) self->__queue, i);  // NOTE: borrowed ref
		if (packed_msg == NULL){
			PyErr_SetString(PyExc_RuntimeError, "Unable to get an item");
			return NULL;
		}

		Py_INCREF(packed_msg);

		msg = PyTuple_GetItem(packed_msg, 0);  // NOTE: borrowed reference
		sub_counter = PyTuple_GetItem(packed_msg, 1);  // NOTE: borrowed reference

		Py_DECREF(packed_msg);
		Py_INCREF(sub_counter);

		is_true = __comparision_c_integer_operator(
			(PyLongObject*) sub_counter, "__eq__", 1, "Unable to compare counter with one",
			"Comparision error"
		);

		if (is_true == -1) {
			Py_DECREF(sub_counter);
			return NULL;
		}
		if (is_true == 1) {
			drop_till += 1;
			Py_DECREF(sub_counter);
			continue;
		}

		if (__reassign_with_c_integer_operator(
			(PyLongObject**) &sub_counter, "__sub__", 1, "Unable to decrease number of subscribers to a message"
		) != 0){
			return NULL;
		}

		Py_DECREF(sub_counter);

		packed_msg = PyTuple_Pack(2, msg, sub_counter);

		if (PySequence_SetItem((PyObject*) self->__queue, i, packed_msg) == -1){
			PyErr_SetString(PyExc_RuntimeError, "Unable to update an item");
			return NULL;
		}
	}

	if (drop_till == 0){
		Py_RETURN_NONE;
	}

	return WMultipleConsumersQueue_Object_clean(self, c_msg_index, c_msg_index + drop_till);
}

static PyObject* WMultipleConsumersQueue_Object_clean(
	WMultipleConsumersQueue_Object* self, Py_ssize_t from_el, Py_ssize_t el_count
) {

	PyObject* msg = NULL;
	PyObject* sub_counter = NULL;
	PyObject* packed_msg = NULL;
	Py_ssize_t i = 0;

	for (i = 0; i < el_count; i++) {

		packed_msg = PyList_GetItem((PyObject*) self->__queue, from_el);  // NOTE: borrowed ref
		if (packed_msg == NULL){
			PyErr_SetString(PyExc_RuntimeError, "Unable to get an item for cleaning");
			return NULL;
		}

		msg = PyTuple_GetItem(packed_msg, 0);  // NOTE: borrowed reference
		sub_counter = PyTuple_GetItem(packed_msg, 1);  // NOTE: borrowed reference

		Py_DECREF(packed_msg);
		Py_DECREF(sub_counter);
		Py_DECREF(msg);

		if (PySequence_DelItem((PyObject*) self->__queue, from_el) == -1){
			PyErr_SetString(PyExc_RuntimeError, "Unable to remove outdated item");
			return NULL;
		}
	}

	if (__reassign_with_c_integer_operator(
		&self->__index_delta, "__add__", el_count, "Unable to increase internal counter"
	) != 0){
		return NULL;
	}

	Py_RETURN_NONE;
}

static PyObject* WMultipleConsumersQueue_Object_push(WMultipleConsumersQueue_Object* self, PyObject* args) {
	__WASP_DEBUG_FN_CALL__;

	PyObject* msg = NULL;
	PyObject* packed_msg = NULL;
	PyObject* callback_result = NULL;
	PyObject* callback_args = NULL;
	int is_true = 0;

	is_true = __comparision_c_integer_operator(
		self->__subscribers, "__le__", 0, "Unable to compare with zero", "Comparision error"
	);

	if (is_true == -1) {
		return NULL;
	}
	else if (is_true == 1) {
		Py_RETURN_NONE;
	}

	if (! PyArg_ParseTuple(args, "O", &msg)){
		PyErr_SetString(PyExc_ValueError, "Message parsing error");
		return NULL;
	}

	Py_INCREF(self->__subscribers);  // NOTE: new item in a tuple
	Py_INCREF(msg);  // NOTE: new item in a tuple
	packed_msg = PyTuple_Pack(2, msg, self->__subscribers);

	if (PyList_Append((PyObject*) self->__queue, packed_msg) != 0){
		Py_DECREF(packed_msg);  // NOTE: clear tuple
		Py_DECREF(msg);  // NOTE: clear item from tuple
		Py_DECREF(self->__subscribers);  // NOTE: clear item from tuple
		return NULL;
	}

	if (self->__callback != NULL){
		callback_args = PyTuple_Pack(0);
		callback_result = PyObject_Call(self->__callback, callback_args, NULL);
		Py_DECREF(callback_args);  // NOTE: does not needed
		if (callback_result == NULL){
			PyErr_SetString(PyExc_RuntimeError, "Callback error!");
			return NULL;
		}
		Py_DECREF(callback_result);  // NOTE: there is not need a call result
	}

	Py_RETURN_NONE;
}

static PyObject* WMultipleConsumersQueue_Object_pop(WMultipleConsumersQueue_Object* self, PyObject* args) {
	__WASP_DEBUG_FN_CALL__;

	PyObject* packed_msg = NULL;
	PyObject* msg = NULL;
	PyObject* sub_counter = NULL;
	PyObject* py_msg_index = NULL;
	int is_true = 0;
	Py_ssize_t c_msg_index = -1;

	py_msg_index = WMultipleConsumersQueue_Object_msg_index(self, args);
	if (py_msg_index == NULL){
		return NULL;
	}

	is_true = WMultipleConsumersQueue_Object_has_raw(self, py_msg_index);

	if (is_true == -1) {
		Py_DECREF(py_msg_index);
		return NULL;
	}
	if (is_true == 0) {
		Py_DECREF(py_msg_index);
		PyErr_SetString(PyExc_KeyError, "No such element found");
		return NULL;
	}

	c_msg_index = PyLong_AsSsize_t(py_msg_index);
	Py_DECREF(py_msg_index);
	if (PyErr_Occurred() != NULL) {
		return NULL;
	}

	packed_msg = PyList_GetItem((PyObject*) self->__queue, c_msg_index);  // NOTE: borrowed ref
	if (packed_msg == NULL){
		return NULL;
	}

	Py_INCREF(packed_msg);

	msg = PyTuple_GetItem(packed_msg, 0);  // NOTE: borrowed reference
	sub_counter = PyTuple_GetItem(packed_msg, 1);  // NOTE: borrowed reference

	Py_DECREF(packed_msg);

	if (msg == NULL || sub_counter == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "Unable to unpack message");
		return NULL;
	}

	is_true = __comparision_c_integer_operator(
		(PyLongObject*) sub_counter, "__eq__", 1, "Unable to compare counter with one", "Comparision error"
	);

	if (is_true == -1) {
		return NULL;
	}
	if (is_true == 1){

		Py_INCREF(msg);

		if (WMultipleConsumersQueue_Object_clean(self, c_msg_index, 1) == NULL){
			return NULL;
		}

		return msg;
	}

	if (__reassign_with_c_integer_operator(
		(PyLongObject**) &sub_counter, "__sub__", 1, "Unable to decrease number of subscribers to a message"
	) != 0){
		return NULL;
	}

	packed_msg = PyTuple_Pack(2, msg, sub_counter);

	if (PySequence_SetItem((PyObject*) self->__queue, c_msg_index, packed_msg) == -1){
		PyErr_SetString(PyExc_RuntimeError, "Unable to update an item");
		return NULL;
	}

	Py_INCREF(msg);
	return msg;
}

static PyObject* WMultipleConsumersQueue_Object_has(WMultipleConsumersQueue_Object* self, PyObject* args) {
	__WASP_DEBUG_FN_CALL__;

	int result = 0;
	PyObject* msg_index = WMultipleConsumersQueue_Object_msg_index(self, args);
	if (msg_index == NULL){
		return NULL;
	}

	result = WMultipleConsumersQueue_Object_has_raw(self, msg_index);
	Py_DECREF(msg_index);

	if (result == -1) {
		return NULL;
	}
	if (result == 0) {
		Py_RETURN_FALSE;
	}
	Py_RETURN_TRUE;
}

static PyObject* WMultipleConsumersQueue_Object_msg_index(WMultipleConsumersQueue_Object* self, PyObject* args) {

	PyObject* msg_index = NULL;
	PyObject* result = NULL;

	if (! PyArg_ParseTuple(args, "O!", &PyLong_Type, &msg_index)){
		PyErr_SetString(PyExc_ValueError, "Argument getting error");
		return NULL;
	}

	Py_INCREF(msg_index);
	result = __py_integer_operator(
		(PyLongObject*) msg_index, "__sub__", (PyObject*) self->__index_delta,
		"Unable to calculate an internal value"
	);
	Py_DECREF(msg_index);

	return result;
}

static int WMultipleConsumersQueue_Object_has_raw(WMultipleConsumersQueue_Object* self, PyObject* msg_index) {
	int is_true = 0;
	Py_ssize_t queue_size = PyList_Size((PyObject *) self->__queue);

	is_true = __comparision_c_integer_operator(
		(PyLongObject*) msg_index, "__ge__", 0, "Unable to compare with zero", "Comparision error"
	);

	if (is_true == 1) {
		return __comparision_c_integer_operator(
			(PyLongObject*) msg_index, "__lt__", queue_size, "Unable to compare with queue size",
			"Comparision error"
		);
	}
	return is_true;
}

static PyObject* WMultipleConsumersQueue_Object_count(WMultipleConsumersQueue_Object* self, PyObject* args) {
	__WASP_DEBUG_FN_CALL__;
	return PyLong_FromSsize_t(PyList_Size((PyObject *) self->__queue));
}
