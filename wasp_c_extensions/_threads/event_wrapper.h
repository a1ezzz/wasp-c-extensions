// wasp_c_extensions/_threads/event_wrapper.h
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

#ifndef __WASP_C_EXTENSIONS__THREADS_EVENT_WRAPPER_H__
#define __WASP_C_EXTENSIONS__THREADS_EVENT_WRAPPER_H__

#include <Python.h>

#include "common.h"

typedef struct {
	PyObject_HEAD
	void* __event;
	PyObject *__weakreflist;
} Event_Object;

PyObject* wasp__threads__Event_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
int wasp__threads__Event_init(Event_Object *self, PyObject *args, PyObject *kwargs);
void wasp__threads__Event_dealloc(Event_Object* self);

PyObject* wasp__threads__Event_wait(Event_Object* self, PyObject* args, PyObject *kwargs);
PyObject* wasp__threads__Event_clear(Event_Object* self, PyObject* args);
PyObject* wasp__threads__Event_set(Event_Object* self, PyObject* args);
PyObject* wasp__threads__Event_is_set(Event_Object* self, PyObject* args);

#endif // __WASP_C_EXTENSIONS__THREADS_EVENT_WRAPPER_H__
