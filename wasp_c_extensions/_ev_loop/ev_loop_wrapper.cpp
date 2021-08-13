// wasp_c_extensions/_ev_loop/ev_loop_wrapper.cpp
//
//Copyright (C) 2021 the wasp-c-extensions authors and contributors
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

extern "C" {
#include "ev_loop_wrapper.h"
#include "_cmcqueue/cmcqueue_wrapper.h"
}

#include "ev_loop.hpp"
#include "_cmcqueue/cmcqueue.hpp"

#ifndef __DEFAULT_SIGNALS_POLLING_TIMEOUT__
#define __DEFAULT_SIGNALS_POLLING_TIMEOUT__ 5 * 1000
#endif

namespace wasp::ev_loop {

typedef EventLoop<PyObject> PyEventLoop;

template<>
void PyEventLoop::call(PyObject* callback){
    Py_INCREF(callback);  // TODO: double check. There is a two INCREF for callback and one only DECREF
    Py_XDECREF(PyObject_CallObject(callback, NULL));  // new ref
    // TODO: throw something if there is an error!
    Py_DECREF(callback);
}

template<>
void PyEventLoop::notify(PyObject* callback){
    Py_INCREF(callback);
    this->notify_impl(callback);
}

template<>
void PyEventLoop::wait_event(){
    __WASP_BEGIN_ALLOW_THREADS__
    EventLoopBase::wait_event();
    __WASP_END_ALLOW_THREADS__
}

};  // namespace wasp::ev_loop

using namespace wasp::ev_loop;

PyObject* wasp__ev_loop__cmcqueue_module = NULL;
PyObject* wasp__ev_loop__cmcqueue_type = NULL;

PyObject* wasp__ev_loop__EventLoop_new(PyTypeObject* type, PyObject* args, PyObject* kwargs){
    EventLoop_Object* self = (EventLoop_Object *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }

    self->__py_queue = NULL;
    self->__event_loop = NULL;

    __WASP_DEBUG__("EventLoop object was allocated");
    return (PyObject *) self;
}

int wasp__ev_loop__EventLoop_init(EventLoop_Object *self, PyObject *args, PyObject *kwargs){
	static const char* kwlist[] = {"py_poll_timeout", "immediate_stop", NULL};
	PyObject *py_poll_timeout = NULL, *py_queue = NULL;
	CMCQueue_Object* c_queue = NULL;
	int c_poll_timeout = -1, immediate_stop_flag = 1;

	if (! PyArg_ParseTupleAndKeywords(args, kwargs, "|Op", (char**) kwlist, &py_poll_timeout, &immediate_stop_flag)){
		return -1;
	}

	if (py_poll_timeout != NULL && py_poll_timeout != Py_None) {
		Py_INCREF(py_poll_timeout);  // NOTE: this ref was not increased by "O"-casting, but it must be
		// since this is a python function argument
		c_poll_timeout = floor(PyFloat_AsDouble(py_poll_timeout) * 1000);  // seconds to milliseconds
		Py_DECREF(py_poll_timeout);  // NOTE: this argument no longer needed
		if (PyErr_Occurred() != NULL) {
			PyErr_SetString(PyExc_ValueError, "'py_poll_timeout' must be able to be converted to C-'double'");
			return -1;
		}
	}
	else {
		c_poll_timeout = __DEFAULT_SIGNALS_POLLING_TIMEOUT__;
	}

    py_queue = PyObject_CallObject(wasp__ev_loop__cmcqueue_type, NULL);  // new ref
    if (! py_queue){
	    PyErr_SetString(PyExc_RuntimeError, "Unable to instantiate a class");
	    return -1;
    }

    self->__py_queue = py_queue;
    c_queue = (CMCQueue_Object*) py_queue;

	self->__event_loop = new PyEventLoop(
	    static_cast<wasp::queue::ICMCQueue*>(c_queue->__queue),
	    std::chrono::milliseconds(c_poll_timeout),
	    immediate_stop_flag
    );

	__WASP_DEBUG__("Event object was initialized");

	return 0;
}

void wasp__ev_loop__EventLoop_dealloc(EventLoop_Object* self){
    if (self->__event_loop){
        delete (static_cast<PyEventLoop*>(self->__event_loop));
        self->__event_loop = NULL;
    }

    if (self->__py_queue){
        Py_DECREF(self->__py_queue);
        self->__py_queue = NULL;
    }

    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyObject* wasp__ev_loop__EventLoop_notify(EventLoop_Object* self, PyObject* args){
    PyObject* callback = NULL;
    if (! PyArg_ParseTuple(args, "O", &callback)){  // "O"-values do not increment ref. counter
        PyErr_SetString(PyExc_ValueError, "Callback parsing error");
        return NULL;
    }

    // TODO: check that callback is callable

    (static_cast<PyEventLoop*>(self->__event_loop))->notify(callback);

    Py_RETURN_NONE;
}

PyObject* wasp__ev_loop__EventLoop_process_event(EventLoop_Object* self, PyObject* args){
    (static_cast<PyEventLoop*>(self->__event_loop))->process_event();
	Py_RETURN_NONE;
}

PyObject* wasp__ev_loop__EventLoop_start_loop(EventLoop_Object* self, PyObject* args){
    (static_cast<PyEventLoop*>(self->__event_loop))->start_loop();
	Py_RETURN_NONE;
}

PyObject* wasp__ev_loop__EventLoop_stop_loop(EventLoop_Object* self, PyObject* args){
    (static_cast<PyEventLoop*>(self->__event_loop))->stop_loop();
	Py_RETURN_NONE;
}
