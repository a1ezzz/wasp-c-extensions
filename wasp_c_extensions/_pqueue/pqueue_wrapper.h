// wasp_c_extensions/_pqueue/pqueue_wrapper.h
//
//Copyright (C) 2022 the wasp-c-extensions authors and contributors
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

#ifndef __WASP_C_EXTENSIONS__PQUEUE_PQUEUE_WRAPPER_H__
#define __WASP_C_EXTENSIONS__PQUEUE_PQUEUE_WRAPPER_H__

#include <Python.h>

#include "common.h"

#define __STR_PQUEUE_NAME__ __STR_FN_CALL__(__PQUEUE_NAME__)

typedef struct {
	PyObject_HEAD
	void* __queue;
} PriorityQueue_Object;

extern PyObject* wasp__pqueue__cgc_module;

PyObject* wasp__pqueue__PriorityQueue_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
void wasp__pqueue__PriorityQueue_dealloc(PriorityQueue_Object* self);

PyObject* wasp__pqueue__PriorityQueue_push(PriorityQueue_Object* self, PyObject* args);
PyObject* wasp__pqueue__PriorityQueue_pull(PriorityQueue_Object* self, PyObject* args);
PyObject* wasp__pqueue__PriorityQueue_next(PriorityQueue_Object* self, PyObject* args);

#endif // __WASP_C_EXTENSIONS__PQUEUE_PQUEUE_WRAPPER_H__
