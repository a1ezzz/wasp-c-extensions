// wasp_c_extensions/_ev_loop/ev_loop_wrapper.h
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

#ifndef __WASP_C_EXTENSIONS__EV_LOOP_EV_LOOP_WRAPPER_H__
#define __WASP_C_EXTENSIONS__EV_LOOP_EV_LOOP_WRAPPER_H__

#include <Python.h>

#include "common.h"

#define __STR_EVENT_LOOP_NAME__ __STR_FN_CALL__(__EVENT_LOOP_NAME__)

typedef struct {
	PyObject_HEAD
	void* __py_queue;
	void* __event_loop;
} EventLoop_Object;

extern PyObject* wasp__ev_loop__cmcqueue_module;
extern PyObject* wasp__ev_loop__cmcqueue_type;

PyObject* wasp__ev_loop__EventLoop_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
int wasp__ev_loop__EventLoop_init(EventLoop_Object *self, PyObject *args, PyObject *kwargs);
void wasp__ev_loop__EventLoop_dealloc(EventLoop_Object* self);

PyObject* wasp__ev_loop__EventLoop_notify(EventLoop_Object* self, PyObject* args);
PyObject* wasp__ev_loop__EventLoop_process_event(EventLoop_Object* self, PyObject* args);
PyObject* wasp__ev_loop__EventLoop_start_loop(EventLoop_Object* self, PyObject* args);
PyObject* wasp__ev_loop__EventLoop_stop_loop(EventLoop_Object* self, PyObject* args);

#endif // __WASP_C_EXTENSIONS__EV_LOOP_EV_LOOP_WRAPPER_H__
