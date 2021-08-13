// wasp_c_extensions/_threads/event_wrapper.cpp
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

#include <cmath>

extern "C" {
#include "event_wrapper.h"
}

#include "event.hpp"

#ifndef __DEFAULT_SIGNALS_POLLING_TIMEOUT__
#define __DEFAULT_SIGNALS_POLLING_TIMEOUT__ 5 * 1000
#endif

using namespace wasp::threads;

PyObject* wasp__threads__Event_new(PyTypeObject* type, PyObject* args, PyObject* kwargs){
    Event_Object* self = (Event_Object *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }
    self->__event = NULL;

    __WASP_DEBUG__("Event object was allocated");
    return (PyObject *) self;
}

int wasp__threads__Event_init(Event_Object *self, PyObject *args, PyObject *kwargs){

	static const char* kwlist[] = {"py_poll_timeout", NULL};
	PyObject* py_poll_timeout = NULL;
	int c_poll_timeout = -1;

	if (! PyArg_ParseTupleAndKeywords(args, kwargs, "|O", (char**) kwlist, &py_poll_timeout)){
		return 0;
	}

	if (py_poll_timeout != NULL && py_poll_timeout != Py_None) {
		Py_INCREF(py_poll_timeout);  // NOTE: this ref was not increased by "O"-casting, but it must be
		// since this is a python function argument
		c_poll_timeout = floor(PyFloat_AsDouble(py_poll_timeout) * 1000);  // seconds to milliseconds
		Py_DECREF(py_poll_timeout);  // NOTE: this argument no longer needed
		if (PyErr_Occurred() != NULL) {
			PyErr_SetString(PyExc_ValueError, "'py_poll_timeout' must be able to be converted to C-'double'");
			return 0;
		}
	}
	else {
		c_poll_timeout = __DEFAULT_SIGNALS_POLLING_TIMEOUT__;
	}

	self->__event = new Event(std::chrono::milliseconds(c_poll_timeout));

	__WASP_DEBUG__("Event object was initialized");

	return 0;
}

void wasp__threads__Event_dealloc(Event_Object* self){

	if (self->__weakreflist != NULL)
        	PyObject_ClearWeakRefs((PyObject *) self);

    if (self->__event){
        delete (static_cast<Event*>(self->__event));
    }
	Py_TYPE(self)->tp_free((PyObject *) self);

	__WASP_DEBUG__("Event object was deallocated");
}

PyObject* wasp__threads__Event_clear(Event_Object* self, PyObject* args){
    (static_cast<Event*>(self->__event))->clear();
    Py_RETURN_NONE;
}

PyObject* wasp__threads__Event_set(Event_Object* self, PyObject* args){
    (static_cast<Event*>(self->__event))->set();
    Py_RETURN_NONE;
}

PyObject* wasp__threads__Event_is_set(Event_Object* self, PyObject* args){
    if ((static_cast<Event*>(self->__event))->is_set()){
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyObject* wasp__threads__Event_wait(Event_Object* self, PyObject* args, PyObject *kwargs){

    std::chrono::time_point<std::chrono::steady_clock> clock;
    std::chrono::milliseconds wait_timeout;
    std::chrono::milliseconds zero_timeout(0);
	PyObject* py_timeout = NULL;
	int c_timeout = -1, py_errors = 0;
	bool wait_result;
	static const char* kwlist[] = {"timeout", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", (char**) kwlist, &py_timeout)){
		return NULL;
	}

	if (py_timeout != NULL && py_timeout != Py_None) {
		Py_INCREF(py_timeout);  // NOTE: '0'-object reference counter must be increased for python function call
	    c_timeout = floor(PyFloat_AsDouble(py_timeout) * 1000);  // seconds to milliseconds
	    Py_DECREF(py_timeout);  // NOTE: function argument no longer needed
		if (PyErr_Occurred() != NULL) {
			PyErr_SetString(PyExc_ValueError, "'timeout' must be able to be converted to C-'double'");
			return NULL;
		}

		clock = std::chrono::steady_clock::now();
    }

	do {
        wait_timeout = std::chrono::milliseconds(c_timeout);

		if (wait_timeout > zero_timeout){
		    wait_timeout -= std::chrono::duration_cast<std::chrono::milliseconds>(
		        std::chrono::steady_clock::now() - clock
		    );
            if (wait_timeout <= zero_timeout){
                break;
            }
		}

		__WASP_BEGIN_ALLOW_THREADS__
        wait_result = (static_cast<Event*>(self->__event))->wait(wait_timeout);
		__WASP_END_ALLOW_THREADS__

		py_errors = PyErr_CheckSignals();

	} while (wait_result == false && py_errors == 0);

	if (py_errors != 0){
		__WASP_DEBUG_PRINTF__("Error condition!");
		return NULL;
	}

	if (wait_result){
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}
