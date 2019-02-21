// wasp_c_extensions/_threads/event.c
//
//Copyright (C) 2018-2019 the wasp-c-extensions authors and contributors
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

#include "event.h"

static PyObject* WPThreadEvent_Type_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
static void WPThreadEvent_Type_dealloc(WPThreadEvent_Object* self);
static int WPThreadEvent_Object_init(WPThreadEvent_Object *self, PyObject *args, PyObject *kwargs);

static PyObject* WPThreadEvent_Object___wait(WPThreadEvent_Object* self, PyObject* timeout);
static PyObject* WPThreadEvent_Object_wait(WPThreadEvent_Object* self, PyObject *args, PyObject *kwargs);
static PyObject* WPThreadEvent_Object_clear(WPThreadEvent_Object* self, PyObject *args);
static PyObject* WPThreadEvent_Object_set(WPThreadEvent_Object* self, PyObject *args);
static PyObject* WPThreadEvent_Object_is_set(WPThreadEvent_Object* self, PyObject *args);

static int increase_timespec(struct timespec* base_value, double increment, struct timespec* result);
static struct timespec* min_timespec(struct timespec* value_a, struct timespec* value_b);


static PyMethodDef WPThreadEvent_Type_methods[] = {

	{
		"wait", (PyCFunction) WPThreadEvent_Object_wait, METH_VARARGS | METH_KEYWORDS,
		"Wait for a event to come. If the event flag was set and is not cleared then this function returns\n"
		"immediately. Returns True if event occurred and False otherwise\n"
		"\n"
		":param timeout: time in seconds during which an event will be awaited (default is no timeout)\n"
		":return: bool"
	},

	{
		"clear", (PyCFunction) WPThreadEvent_Object_clear, METH_NOARGS,
		"Clear the event flag\n"
		"\n"
		":return: None"

	},

	{
		"set", (PyCFunction) WPThreadEvent_Object_set, METH_NOARGS,
		"Set the event flag\n"
		"\n"
		":return: None"
	},

	{
		"is_set", (PyCFunction) WPThreadEvent_Object_is_set, METH_NOARGS,
		"Return the event flag state. True if this flag is set, False - otherwise\n"
		"\n"
		":return: bool"

	},

	{NULL}
};

PyTypeObject WPThreadEvent_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = __STR_PACKAGE_NAME__"."__STR_THREADS_MODULE_NAME__"."__STR_PTHREAD_EVENT_NAME__,
	.tp_doc = "threading.Event a-like object implemented with Linux pthread library",
	.tp_basicsize = sizeof(WPThreadEvent_Type),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = WPThreadEvent_Type_new,
	.tp_init = (initproc) WPThreadEvent_Object_init,
	.tp_dealloc = (destructor) WPThreadEvent_Type_dealloc,
	.tp_methods = WPThreadEvent_Type_methods,
	.tp_weaklistoffset = offsetof(WPThreadEvent_Object, __weakreflist)
};

static PyObject* WPThreadEvent_Type_new(PyTypeObject* type, PyObject* args, PyObject* kwargs) {

	__WASP_DEBUG_FN_CALL__;

	WPThreadEvent_Object* self;
	self = (WPThreadEvent_Object *) type->tp_alloc(type, 0);

	if (self == NULL) {
		return PyErr_NoMemory();
	}

	self->__is_set = false;

	if (pthread_mutex_init(&self->__mutex, NULL) != 0){
		Py_DECREF(self);
		PyErr_SetString(PyExc_RuntimeError, "Unable to allocate mutex");
		return NULL;
	}

	if (pthread_cond_init(&self->__conditional_variable, NULL) != 0){
		Py_DECREF(self);
		PyErr_SetString(PyExc_RuntimeError, "Unable to allocate conditional variable");
		return NULL;
	}

	__WASP_DEBUG_PRINTF__("Object \""__STR_PTHREAD_EVENT_NAME__"\" was allocated");

	return (PyObject *) self;
}

static void WPThreadEvent_Type_dealloc(WPThreadEvent_Object* self) {

	__WASP_DEBUG_FN_CALL__;

	if (self->__weakreflist != NULL)
        	PyObject_ClearWeakRefs((PyObject *) self);

	if (pthread_mutex_destroy(&self->__mutex) != 0){
		PyErr_WriteUnraisable((PyObject*) self);
	}

	if (pthread_cond_destroy(&self->__conditional_variable) != 0){
		PyErr_WriteUnraisable((PyObject*) self);
	}

	Py_TYPE(self)->tp_free((PyObject *) self);

	__WASP_DEBUG_PRINTF__("Object \""__STR_PTHREAD_EVENT_NAME__"\" was deallocated");
}

static int WPThreadEvent_Object_init(WPThreadEvent_Object *self, PyObject *args, PyObject *kwargs) {

	__WASP_DEBUG_FN_CALL__;

	static char *kwlist[] = {"py_error_poll_timeout", NULL};
	PyObject* py_error_timeout = NULL;
	double c_error_timeout = NAN;

	if (! PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &py_error_timeout)){
		return 0;
	}

	if (py_error_timeout != NULL && py_error_timeout != Py_None) {

		Py_INCREF(py_error_timeout);  // NOTE: this ref was not increased by "O"-casting, but it must be
		// since this is a python function argument

		c_error_timeout = PyFloat_AsDouble(py_error_timeout);

		Py_DECREF(py_error_timeout);  // NOTE: this argument no longer needed

		if (PyErr_Occurred() != NULL) {
			PyErr_SetString(
				PyExc_ValueError, "'py_error_poll_timeout' must be able to be converted to C-'double'"
			);
			return 0;
		}
		self->error_poll_timeout = c_error_timeout;
	}
	else {
		self->error_poll_timeout = __DEFAULT_SIGNALS_POLLING_TIMEOUT__;
	}

	__WASP_DEBUG_PRINTF__("Object \""__STR_PTHREAD_EVENT_NAME__"\" was initialized");

	return 0;
}

static PyObject* WPThreadEvent_Object_wait(WPThreadEvent_Object* self, PyObject *args, PyObject *kwargs) {

	__WASP_DEBUG_FN_CALL__;

	PyObject* py_timeout = NULL;
	static char *kwlist[] = {"timeout", NULL};

	__WASP_DEBUG_PRINTF__("Parsing arguments");

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &py_timeout)){
		return NULL;
	}

	__WASP_DEBUG_PRINTF__("Arguments were parsed");

	return WPThreadEvent_Object___wait(self, py_timeout);
}

static PyObject* WPThreadEvent_Object___wait(WPThreadEvent_Object* self, PyObject* py_timeout) {

	__WASP_DEBUG_FN_CALL__;

	double c_timeout = NAN;
	struct timespec max_poll_time = { .tv_sec = 0, .tv_nsec = 0 }, next_poll_time = { .tv_sec = 0, .tv_nsec = 0 };
	struct timespec* next_time = 0;
	int wait_result = 0, py_errors = 0;

	if (self->__is_set){
		Py_RETURN_TRUE;
	}

	if (py_timeout != NULL && py_timeout != Py_None) {

		__WASP_DEBUG_PRINTF__("A timeout was specified");

		Py_INCREF(py_timeout);  // NOTE: '0'-object reference counter must be increased for python function call
		c_timeout = PyFloat_AsDouble(py_timeout);
		Py_DECREF(py_timeout);  // NOTE: function argument no longer needed
		if (PyErr_Occurred() != NULL) {
			PyErr_SetString(PyExc_ValueError, "'timeout' must be able to be converted to C-'double'");
			return NULL;
		}

		__WASP_DEBUG_PRINTF__("Parsed timeout values is: %f", c_timeout);
	}

	__WASP_DEBUG_PRINTF__("Calculating timeouts");

	clock_gettime(CLOCK_REALTIME, &next_poll_time);

	if (! isnan(c_timeout)) {
		if (increase_timespec(&next_poll_time, c_timeout, &max_poll_time) != 0){
			__WASP_DEBUG_PRINTF__("Raising an exception!");
			PyErr_SetString(PyExc_RuntimeError, "The specified polling date time is out of range");
			return NULL;
		}

	}

	if (increase_timespec(&next_poll_time, self->error_poll_timeout, &next_poll_time) != 0){
		__WASP_DEBUG_PRINTF__("Raising an exception!");
		PyErr_SetString(PyExc_RuntimeError, "The internal polling date time is out of range");
		return NULL;
	}

	do {
		__WASP_BEGIN_ALLOW_THREADS__

		if (!isnan(c_timeout)){
			next_time = min_timespec(&max_poll_time, &next_poll_time);
		}
		else {
			next_time = &next_poll_time;
		}

		__WASP_DEBUG_PRINTF__("Waiting for a conditional variable");
		pthread_mutex_lock(&self->__mutex);

		if (! self->__is_set){
			wait_result = pthread_cond_timedwait(&self->__conditional_variable, &self->__mutex, next_time);
		}

		pthread_mutex_unlock(&self->__mutex);
		__WASP_END_ALLOW_THREADS__

		if (self->__is_set){
			Py_RETURN_TRUE;
		}

		if (wait_result == 0){
			__WASP_DEBUG_PRINTF__("Conditional variable was set");
			break;
		}
		else if (wait_result == ETIMEDOUT){

			__WASP_DEBUG_PRINTF__("Conditional variable was not set during an iteration timeout");
			if (next_time == (&max_poll_time)){
				__WASP_DEBUG_PRINTF__("Conditional variable was not set during the specified timeout");
				break;
			}
		}
		else {
			__WASP_DEBUG_PRINTF__("Conditional variable was not set and unknown error spotted");
			PyErr_SetString(PyExc_RuntimeError, "An unhandled error spotted during time wait");
			break;
		}

		clock_gettime(CLOCK_REALTIME, &next_poll_time);

		if (increase_timespec(&next_poll_time, self->error_poll_timeout, &next_poll_time) != 0){
			__WASP_DEBUG_PRINTF__("Raising an exception!");
			PyErr_SetString(PyExc_RuntimeError, "The next polling date time is out of range");
			return NULL;
		}

		__WASP_DEBUG_PRINTF__("We are ready for the next iteration");

		py_errors = PyErr_CheckSignals();

	} while (py_errors == 0);

	if (py_errors != 0){
		__WASP_DEBUG_PRINTF__("Error condition!");
		return NULL;
	}

	if (self->__is_set){
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

static PyObject* WPThreadEvent_Object_clear(WPThreadEvent_Object* self, PyObject *args) {
	__WASP_DEBUG_PRINTF__("A call to \""__STR_PTHREAD_EVENT_NAME__".clear\" method was made");
	self->__is_set = false;
	Py_RETURN_FALSE;
}

static PyObject* WPThreadEvent_Object_set(WPThreadEvent_Object* self, PyObject *args) {
	__WASP_DEBUG_PRINTF__("A call to \""__STR_PTHREAD_EVENT_NAME__".set\" method was made");

	if (self->__is_set){
		__WASP_DEBUG_PRINTF__(
			"Internal flag of a \""__STR_PTHREAD_EVENT_NAME__"\" object has been already set"
		);
		Py_RETURN_NONE;
	}

	__WASP_BEGIN_ALLOW_THREADS__

	__WASP_DEBUG_PRINTF__("Acquiring a lock");
	pthread_mutex_lock(&self->__mutex);

	self->__is_set = true;

	__WASP_DEBUG_PRINTF__("Notifying a conditional variable");
	pthread_cond_broadcast(&self->__conditional_variable);

	__WASP_DEBUG_PRINTF__("Freeing a lock");
	pthread_mutex_unlock(&self->__mutex);

	__WASP_END_ALLOW_THREADS__

	Py_RETURN_NONE;
}


static PyObject* WPThreadEvent_Object_is_set(WPThreadEvent_Object* self, PyObject *args)
{
	__WASP_DEBUG_PRINTF__("A call to \""__STR_PTHREAD_EVENT_NAME__".is_set\" method was made");
	if (self->__is_set){
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

static int increase_timespec(struct timespec* base_value, double increment, struct timespec* result)
{
	double nsec_in_sec = pow(10, 9);
	double seconds = floor(increment);
	double nano_seconds = ((increment - seconds) * nsec_in_sec) + base_value->tv_nsec;

	if (nano_seconds > nsec_in_sec){
		nano_seconds = fmod(nano_seconds, nsec_in_sec);
		seconds += 1;
	}
	nano_seconds = ceil(nano_seconds);
	seconds += base_value->tv_sec;

	if ((seconds) > LONG_MAX) {
		return -1;
	}

	result->tv_sec = ((time_t) seconds);
	result->tv_nsec = ((long) nano_seconds);

	return 0;
}

static struct timespec* min_timespec(struct timespec* value_a, struct timespec* value_b)
{
	if (
		(value_b->tv_sec < value_a->tv_sec) ||
		((value_b->tv_sec == value_a->tv_sec) && (value_b->tv_nsec < value_a->tv_nsec))
	){
		return value_b;
	}
	return value_a;
}
