// wasp_c_extensions/_queue/subscriber.c
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

#include "subscriber.h"
#include "_common/static_functions.h"

static PyObject* WMCQueueSubscriber_Type_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
static void WMCQueueSubscriber_Type_dealloc(WMCQueueSubscriber_Object* self);
static int WMCQueueSubscriber_Object_init(WMCQueueSubscriber_Object *self, PyObject *args, PyObject *kwargs);

static PyObject* WMCQueueSubscriber_Object_subscribed(WMCQueueSubscriber_Object* self, PyObject* args);
static PyObject* WMCQueueSubscriber_Object_next(WMCQueueSubscriber_Object* self, PyObject* args);
static PyObject* WMCQueueSubscriber_Object_has_next(WMCQueueSubscriber_Object* self, PyObject* args);
static PyObject* WMCQueueSubscriber_Object_unsubscribe(WMCQueueSubscriber_Object* self, PyObject* args);

static PyMethodDef WMCQueueSubscriber_Type_methods[] = {
	{
		"next", (PyCFunction) WMCQueueSubscriber_Object_next, METH_NOARGS,
		"Return next message. If a message is not ready then KeyError exception will be raised\n"
		"\n"
		":rtype: anything\n"
	},
	{
		"subscribed", (PyCFunction) WMCQueueSubscriber_Object_subscribed, METH_NOARGS,
		"Return True if this subscriber is actual and was not unsubscribed\n"
		"\n"
		":rtype: bool\n"
	},
	{
		"has_next", (PyCFunction) WMCQueueSubscriber_Object_has_next, METH_NOARGS,
		"Check if there is a new message available in a queue\n"
		"\n"
		":rtype: bool\n"
	},
	{
		"unsubscribe", (PyCFunction) WMCQueueSubscriber_Object_unsubscribe, METH_NOARGS,
		"Unsubscribe from a queue. No more messages will be available from a queue\n"
		"\n"
		":rtype: None\n"
	},
	{NULL}
};

PyTypeObject WMCQueueSubscriber_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = __STR_PACKAGE_NAME__"."__STR_QUEUE_MODULE_NAME__"."__STR_MCQUEUE_SUBSCRIBER_NAME__,
	.tp_doc = "This is a simple thread-safe subscriber for \""__STR_MCQUEUE_NAME__"\" class. It wraps subscription "
	"and unsubscription procedures and wraps message index handling.",
	.tp_basicsize = sizeof(WMCQueueSubscriber_Type),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_new = WMCQueueSubscriber_Type_new,
	.tp_init = (initproc) WMCQueueSubscriber_Object_init,
	.tp_dealloc = (destructor) WMCQueueSubscriber_Type_dealloc,
	.tp_methods = WMCQueueSubscriber_Type_methods,
	.tp_weaklistoffset = offsetof(WMCQueueSubscriber_Object, __weakreflist)
};

static PyObject* WMCQueueSubscriber_Type_new(PyTypeObject* type, PyObject* args, PyObject* kwargs) {
	__WASP_DEBUG_FN_CALL__;

	WMCQueueSubscriber_Object* self;
	self = (WMCQueueSubscriber_Object *) type->tp_alloc(type, 0);
	if (self == NULL) {
		return PyErr_NoMemory();
	}

	self->__queue = NULL;
	self->__msg_index = NULL;

	__WASP_DEBUG_PRINTF__("Object \""__STR_MCQUEUE_SUBSCRIBER_NAME__"\" was allocated");

	return (PyObject *) self;
}

static void WMCQueueSubscriber_Type_dealloc(WMCQueueSubscriber_Object* self) {
	__WASP_DEBUG_FN_CALL__;

	if (self->__weakreflist != NULL) {
        	PyObject_ClearWeakRefs((PyObject *) self);
        }

	if (self->__queue != NULL && self->__msg_index != NULL) {
		WMCQueueSubscriber_Object_unsubscribe(self, NULL);
	}

	if (self->__queue != NULL) {
		Py_DECREF(self->__queue);
	}

	if (self->__msg_index != NULL) {
		Py_DECREF(self->__msg_index);
	}

	Py_TYPE(self)->tp_free((PyObject *) self);

	__WASP_DEBUG_PRINTF__("Object \""__STR_MCQUEUE_SUBSCRIBER_NAME__"\" was deallocated");
}

static int WMCQueueSubscriber_Object_init(WMCQueueSubscriber_Object *self, PyObject *args, PyObject *kwargs) {
	__WASP_DEBUG_FN_CALL__;

	PyObject* queue = NULL;
	PyObject* msg_index = NULL;

	if (! PyArg_ParseTuple(args, "O!", &WMultipleConsumersQueue_Type, &queue)) {
		return -1;
	}

	Py_INCREF(queue);

	msg_index = PyObject_CallMethod((PyObject *) queue, "subscribe", NULL);
	if (msg_index == NULL){
		if (PyErr_Occurred() == NULL) {
			PyErr_SetString(PyExc_RuntimeError, "Unable to subscribe to the queue!");
		}
		Py_DECREF(queue);
		return -1;
	}

	self->__queue = queue;
	self->__msg_index = (PyLongObject*) msg_index;

	__WASP_DEBUG_PRINTF__("Object \""__STR_MCQUEUE_SUBSCRIBER_NAME__"\" was initialized");
	return 0;
}

static PyObject* WMCQueueSubscriber_Object_subscribed(WMCQueueSubscriber_Object* self, PyObject* args) {
	if (self->__queue != NULL && self->__msg_index != NULL){
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

static PyObject* WMCQueueSubscriber_Object_next(WMCQueueSubscriber_Object* self, PyObject* args) {
	__WASP_DEBUG_FN_CALL__;

	PyObject* msg = NULL;

	if (self->__queue == NULL || self->__msg_index == NULL){
		PyErr_SetString(PyExc_RuntimeError, "Unable to get next message because subscriber was unsubscribe!");
		return NULL;
	}

	msg = PyObject_CallMethod((PyObject *) self->__queue, "pop", "(O)", self->__msg_index);
	if (msg == NULL){
		if (PyErr_Occurred() == NULL) {
			PyErr_SetString(PyExc_RuntimeError, "Unable to get message from a queue. Internal error");
		}
		return NULL;
	}

	if (__reassign_with_c_integer_operator(
		&self->__msg_index, "__add__", 1, "Unable to increase internal counter"
	) != 0){
		return NULL;
	}

	return msg;
}

static PyObject* WMCQueueSubscriber_Object_has_next(WMCQueueSubscriber_Object* self, PyObject* args) {
	__WASP_DEBUG_FN_CALL__;

	if (self->__queue == NULL || self->__msg_index == NULL){
		PyErr_SetString(PyExc_RuntimeError, "Unable to get next message because subscriber was unsubscribe!");
		return NULL;
	}

	return PyObject_CallMethod((PyObject *) self->__queue, "has", "(O)", self->__msg_index);
}

static PyObject* WMCQueueSubscriber_Object_unsubscribe(WMCQueueSubscriber_Object* self, PyObject* args) {
	__WASP_DEBUG_FN_CALL__;

	PyObject* result = NULL;

	if (self->__queue == NULL || self->__msg_index == NULL){
		PyErr_SetString(PyExc_RuntimeError, "Unable to unsubscribe subscriber because it unsubscribed before!");
		return NULL;
	}

	result = PyObject_CallMethod((PyObject *) self->__queue, "unsubscribe", "(O)", self->__msg_index);
	if (result == NULL){
		if (PyErr_Occurred() == NULL) {
			PyErr_SetString(PyExc_RuntimeError, "Unable to unsubscribe from a queue. Internal error");
		}
		return NULL;
	}

	Py_DECREF(self->__queue);
	Py_DECREF(self->__msg_index);

	self->__queue = NULL;
	self->__msg_index = NULL;

	return result;
}
