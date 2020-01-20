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

#include <stdbool.h>
#include "mcqueue.h"

static PyObject* WMultipleConsumersQueue_Type_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
static void WMultipleConsumersQueue_Type_dealloc(WMultipleConsumersQueue_Object* self);
static int WMultipleConsumersQueue_Object_init(WMultipleConsumersQueue_Object *self, PyObject *args, PyObject *kwargs);

static PyObject* WMultipleConsumersQueue_Object_subscribe(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_unsubscribe(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_push(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_pop(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_has(WMultipleConsumersQueue_Object* self, PyObject* args);
static int WMultipleConsumersQueue_Object_args_index(
    WMultipleConsumersQueue_Object* self, PyObject* args, Py_ssize_t* args_index, bool* valid_index
);
static PyObject* WMultipleConsumersQueue_Object_count(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_clean(
	WMultipleConsumersQueue_Object* self, Py_ssize_t from_el, Py_ssize_t el_count
);
static PyObject* WMultipleConsumersQueue_Object_msg(
    WMultipleConsumersQueue_Object* self, Py_ssize_t msg_index, Py_ssize_t* sub_counter
);
static PyObject* WMultipleConsumersQueue_Object_packed_msg(PyObject* msg, Py_ssize_t sub_counter);
static int WMultipleConsumersQueue_Object_insert_packed_msg(
    WMultipleConsumersQueue_Object* self, Py_ssize_t msg_index, PyObject* msg, Py_ssize_t sub_counter
);
static int WMultipleConsumersQueue_Object_append_packed_msg(
    WMultipleConsumersQueue_Object* self, PyObject* msg, Py_ssize_t sub_counter
);

static PyMethodDef WMultipleConsumersQueue_Type_methods[] = {
	{
		"subscribe", (PyCFunction) WMultipleConsumersQueue_Object_subscribe, METH_NOARGS,
		"\"Subscribe\" to this queue and return index of the next message.\n"
		"\n"
		":return: int"
	},
	{
		"unsubscribe", (PyCFunction) WMultipleConsumersQueue_Object_unsubscribe, METH_VARARGS,
		"\"Unsubscribe\" from a queue. In order to unsubscribe subscriber must submit its message index. "
		"All the messages from that index and further won't be available to this subscriber any more.\n"
		"\n"
		":param msg_index: id of subscriber message\n"
		":return: None"
	},
	{
		"push", (PyCFunction) WMultipleConsumersQueue_Object_push, METH_VARARGS,
		"Send a new message to this queue\n"
		"\n"
		":param msg: message to send\n"
		":return: None"
	},
	{
		"pop", (PyCFunction) WMultipleConsumersQueue_Object_pop, METH_VARARGS,
		"Get message from a queue by an index. Must be:\n"
		" - called by a subscriber\n"
		" - \"msg_index\" parameter must be subscribers index\n"
		" - subscriber index must increase its index by one after message retrieving\n"
		"Same subscriber must not request the same message twice!\n"
		"\n"
		":param msg_index: id of a message to return\n"
		":return: message that was pushed"
	},
	{
		"has", (PyCFunction) WMultipleConsumersQueue_Object_has, METH_VARARGS,
		"Check if there is a message with index in a queue. Should be used by subscribers only for checking "
		"new messages\n"
		"\n"
		":param msg_index: id of a message to check\n"
		":return: bool"
	},
	{
		"count", (PyCFunction) WMultipleConsumersQueue_Object_count, METH_NOARGS,
		"Return number of messages that this queue has. This method should be used for a queue diagnostic "
		"only. And it must not be used for determining existing message indexes.\n"
		"\n"
		":return: int"
	},
	{NULL}
};

PyTypeObject WMultipleConsumersQueue_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = __STR_PACKAGE_NAME__"."__STR_QUEUE_MODULE_NAME__"."__STR_MCQUEUE_NAME__,
	.tp_doc = "This is a simple queue that allows multiple consumers get their own copy of incoming data.\n"
	"There are not many checks in this implementation. Subscribers behaviour is not restricted but they should "
	"play nice in order to make this queue consistent.\n"
	"Anything may send any data (it calls a message) to this queue but in order to receive that data (message) "
	"from a queue subscription must be made. If there are no subscribers then no messages will be saved for the "
	"following receiving.\n"
	"Lets assume that there is one subscriber at least. Then a new message may be pushed to a queue. That message "
	"won't be erased until all current subscribers get that message (or until some part or all of the subscribers "
	"unsubscribe from this queue)\n"
	"In order to subscribe \""__STR_MCQUEUE_NAME__".subscribe\" method must be called. As a result it returns an "
	"index of the next message (a message that does not exist at the moment of subscription but that will be "
	"available on the next message pushing). That index must be saved by a subscriber and with that index "
	"the next message may be retrieved. After retrieving of the message subscriber must increase his index by one. "
	"If there are no need in new messages then subscriber must unsubscribe itself from a queue\n"
	"As it was written before. A queue will be consistent if previous rules are followed",
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

	WMultipleConsumersQueue_Object* self = (WMultipleConsumersQueue_Object *) type->tp_alloc(type, 0);

	if (self == NULL) {
		return PyErr_NoMemory();
	}

	self->__callback = NULL;
	self->__index_delta = (PyLongObject*) PyLong_FromLong(0);
	self->__subscribers = 0;
	self->__queue = (PyListObject*) PyList_New(0);

    if (
        self->__index_delta == NULL ||
        self->__queue == NULL
    ){
        Py_XDECREF(self->__callback);
        Py_XDECREF(self->__index_delta);
        Py_XDECREF(self->__queue);
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

	if (callback != NULL && callback != Py_None){
		Py_INCREF(callback);  // NOTE: 'O'-object refs counters must be increased
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

	Py_ssize_t queue_size = 0;

	if (self->__weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }

	if (self->__queue != NULL){
	    queue_size = PyList_Size((PyObject *) self->__queue);
		__WASP_DEBUG_PRINTF__("Cleaning queue (current size %i)", queue_size);
		WMultipleConsumersQueue_Object_clean(self, 0, queue_size);
		Py_DECREF(self->__queue);
	}

    Py_XDECREF(self->__index_delta);
    Py_XDECREF(self->__callback);

	Py_TYPE(self)->tp_free((PyObject *) self);

	__WASP_DEBUG_PRINTF__("Object \""__STR_MCQUEUE_NAME__"\" was deallocated");
}

static PyObject* WMultipleConsumersQueue_Object_clean(
	WMultipleConsumersQueue_Object* self, Py_ssize_t from_el, Py_ssize_t el_count
) {
	__WASP_DEBUG_FN_CALL__;

    PyObject* py_el_count = NULL;
    PyLongObject* new_delta = NULL;
	Py_ssize_t i = 0;

	for (i = 0; i < el_count; i++) {

		if (PySequence_DelItem((PyObject*) self->__queue, from_el) == -1){
			PyErr_SetString(PyExc_RuntimeError, "Unable to remove outdated item");
			return NULL;
		}
	}

    py_el_count = PyLong_FromSsize_t(el_count); // new ref
	new_delta = (PyLongObject*) PyLong_Type.tp_as_number->nb_add(
	    (PyObject*) self->__index_delta, py_el_count
	);  // new ref or NULL
    Py_XDECREF(py_el_count);
    if (new_delta != NULL) {
        Py_DECREF(self->__index_delta);
        self->__index_delta = new_delta;
        Py_RETURN_NONE;
    }

    Py_XDECREF(new_delta);
    PyErr_SetString(PyExc_RuntimeError, "Unable to increase internal counter");
	return NULL;
}

static PyObject* WMultipleConsumersQueue_Object_subscribe(WMultipleConsumersQueue_Object* self, PyObject* args){
	__WASP_DEBUG_FN_CALL__;

    PyObject* queue_size = PyLong_FromSsize_t(PyList_Size((PyObject *) self->__queue)); // new ref
    PyObject* next_message = PyLong_Type.tp_as_number->nb_add((PyObject*) self->__index_delta, queue_size);  // new ref

    if (queue_size == NULL || next_message == NULL){
        Py_XDECREF(queue_size);
        Py_XDECREF(next_message);

		PyErr_SetString(PyExc_RuntimeError, "Unable to subscribe");
		return NULL;
    }

    Py_DECREF(queue_size);
    self->__subscribers += 1;
    return next_message;
}

static int WMultipleConsumersQueue_Object_args_index(
    WMultipleConsumersQueue_Object* self, PyObject* args, Py_ssize_t* args_index, bool* valid_index
) {
	PyObject* py_args_index = NULL;
	PyObject* queue_index = NULL;
	Py_ssize_t queue_size = PyList_Size((PyObject *) self->__queue);

    assert(args_index != NULL);

	if (! PyArg_ParseTuple(args, "O!", &PyLong_Type, &py_args_index)){
		PyErr_SetString(PyExc_ValueError, "Argument parsing error");
		return -1;
	}

	Py_INCREF(py_args_index);
    queue_index = PyNumber_Subtract(py_args_index, (PyObject*) self->__index_delta);  // new ref or NULL
	Py_DECREF(py_args_index);

    if (queue_index != NULL) {
        (*args_index) = PyLong_AsSsize_t(queue_index);
        Py_DECREF(queue_index);

        if (valid_index != NULL) {
            (*valid_index) = ((*args_index) >= 0 && ((*args_index) < queue_size)) ? true : false;
        }
        return 0;
    }

    PyErr_SetString(PyExc_RuntimeError, "Unable to find an index");
	return -1;
}

static PyObject* WMultipleConsumersQueue_Object_msg(
    WMultipleConsumersQueue_Object* self, Py_ssize_t msg_index, Py_ssize_t* sub_counter
) {
	PyObject* packed_msg = NULL;
	PyObject* msg = NULL;
	Py_ssize_t c = 0;
	int op_status = 0;

	if ((packed_msg = PyList_GetItem((PyObject*) self->__queue, msg_index)) == NULL) {  // NOTE: borrowed ref
		return NULL;
	}

	Py_INCREF(packed_msg);
	op_status = PyArg_ParseTuple(packed_msg, "On", &msg, &c);
    Py_DECREF(packed_msg);
	if (! op_status) {
        PyErr_SetString(PyExc_KeyError, "Unable to parse a message");
        return NULL;
    }

    if (sub_counter != NULL) {
        (*sub_counter) = c;
    }

	Py_INCREF(msg);
    return msg;
}

static PyObject* WMultipleConsumersQueue_Object_packed_msg(PyObject* msg, Py_ssize_t sub_counter){
	PyObject* packed_msg = NULL;

    packed_msg = Py_BuildValue("On", msg, sub_counter); // new ref
	if (packed_msg == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "Unable to pack a new message");
	}
	return packed_msg;
}

static int WMultipleConsumersQueue_Object_insert_packed_msg(
    WMultipleConsumersQueue_Object* self, Py_ssize_t msg_index, PyObject* msg, Py_ssize_t sub_counter
) {
	PyObject* packed_msg = WMultipleConsumersQueue_Object_packed_msg(msg, sub_counter);
    if (packed_msg == NULL){
        return -1;
    }

	if (PySequence_SetItem((PyObject*) self->__queue, msg_index, packed_msg) == -1){
        Py_DECREF(packed_msg);
		PyErr_SetString(PyExc_RuntimeError, "Unable to update an item");
		return -1;
	}

	return 0;
}

static int WMultipleConsumersQueue_Object_append_packed_msg(
    WMultipleConsumersQueue_Object* self, PyObject* msg, Py_ssize_t sub_counter
) {
	PyObject* packed_msg = WMultipleConsumersQueue_Object_packed_msg(msg, sub_counter);
    if (packed_msg == NULL){
        return -1;
    }

	if (PyList_Append((PyObject*) self->__queue, packed_msg) != 0){
		Py_DECREF(packed_msg);
		return -1;
	}

	return 0;
}

static PyObject* WMultipleConsumersQueue_Object_pop(WMultipleConsumersQueue_Object* self, PyObject* args) {
	__WASP_DEBUG_FN_CALL__;

	PyObject* msg = NULL;
	Py_ssize_t sub_counter = 0;
	Py_ssize_t msg_index = -1;
	bool index_valid = false;

	if (WMultipleConsumersQueue_Object_args_index(self, args, &msg_index, &index_valid) != 0){
		return NULL;
	}

    if (index_valid == false) {
		PyErr_SetString(PyExc_KeyError, "No such element found");
		return NULL;
    }

	if ((msg = WMultipleConsumersQueue_Object_msg(self, msg_index, &sub_counter)) == NULL){
	    return NULL;
	}

    assert(sub_counter > 0);

    if (sub_counter == 1) {
		if (WMultipleConsumersQueue_Object_clean(self, msg_index, 1) == NULL){
			Py_DECREF(msg);
			return NULL;
		}
        return msg;
    }

    if (WMultipleConsumersQueue_Object_insert_packed_msg(self, msg_index, msg, sub_counter - 1) != 0){
        Py_DECREF(msg);
        return NULL;
    }

	return msg;
}

static PyObject* WMultipleConsumersQueue_Object_unsubscribe(WMultipleConsumersQueue_Object* self, PyObject* args) {

	__WASP_DEBUG_FN_CALL__;

	PyObject* msg = NULL;
	Py_ssize_t sub_counter = 0;
	Py_ssize_t msg_index = 0;
	Py_ssize_t queue_length = PyList_Size((PyObject *) self->__queue);
	Py_ssize_t drop_till = 0;
	Py_ssize_t i = 0;
	int op_status = 0;

    if (WMultipleConsumersQueue_Object_args_index(self, args, &msg_index, NULL) != 0) {
		return NULL;
    }

    self->__subscribers -= 1;

	for (i = msg_index; i < queue_length; i++) {

		if ((msg = WMultipleConsumersQueue_Object_msg(self, i, &sub_counter)) == NULL){
	        return NULL;
		}

		if (sub_counter == 1){
			Py_DECREF(msg);
			drop_till += 1;
			continue;
		}

        op_status = WMultipleConsumersQueue_Object_insert_packed_msg(self, i, msg, sub_counter - 1);
        Py_DECREF(msg);
        if (op_status != 0){
            return NULL;
        }
	}

	if (drop_till == 0){
		Py_RETURN_NONE;
	}

	return WMultipleConsumersQueue_Object_clean(self, msg_index, msg_index + drop_till);
}

static PyObject* WMultipleConsumersQueue_Object_push(WMultipleConsumersQueue_Object* self, PyObject* args) {
	__WASP_DEBUG_FN_CALL__;

	PyObject* msg = NULL;
	PyObject* callback_result = NULL;
	PyObject* callback_args = NULL;

    if (self->__subscribers == 0){
	    // do nothing since there are no subscribers
		Py_RETURN_NONE;
	}

	if (! PyArg_ParseTuple(args, "O", &msg)){
		PyErr_SetString(PyExc_ValueError, "Message parsing error");
		return NULL;
	}

    if (WMultipleConsumersQueue_Object_append_packed_msg(self, msg, self->__subscribers) != 0){
        return NULL;
    }

	if (self->__callback != NULL){
		callback_args = PyTuple_Pack(0);
		callback_result = PyObject_Call(self->__callback, callback_args, NULL);
		Py_DECREF(callback_args);  // NOTE: does not needed
		if (callback_result == NULL){
			if (PyErr_Occurred() == NULL) {
				PyErr_SetString(PyExc_RuntimeError, "Callback error!");
			}
			return NULL;
		}
		Py_DECREF(callback_result);  // NOTE: a call result is not needed
	}

	Py_RETURN_NONE;
}

static PyObject* WMultipleConsumersQueue_Object_has(WMultipleConsumersQueue_Object* self, PyObject* args) {
	__WASP_DEBUG_FN_CALL__;

	Py_ssize_t msg_index = 0;
	bool valid_index = false;

    if (WMultipleConsumersQueue_Object_args_index(self, args, &msg_index, &valid_index) != 0) {
		return NULL;
    }

    if (valid_index) {
        Py_RETURN_TRUE;
    }

    Py_RETURN_FALSE;
}

static PyObject* WMultipleConsumersQueue_Object_count(WMultipleConsumersQueue_Object* self, PyObject* args) {
	__WASP_DEBUG_FN_CALL__;
	return PyLong_FromSsize_t(PyList_Size((PyObject *) self->__queue));
}
