// wasp_c_extensions/_ollist/ollist_wrapper.h
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

#ifndef __WASP_C_EXTENSIONS__OLLIST_OLLIST_WRAPPER_H__
#define __WASP_C_EXTENSIONS__OLLIST_OLLIST_WRAPPER_H__

#include <Python.h>

#include "common.h"

#define __STR_OLLIST_NAME__ __STR_FN_CALL__(__OLLIST_NAME__)

typedef struct {
	PyObject_HEAD
	void* __list;
} OrderedLinkedList_Object;

PyObject* wasp__ollist__OrderedLinkedList_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
void wasp__ollist__OrderedLinkedList_dealloc(OrderedLinkedList_Object* self);

PyObject* wasp__ollist__OrderedLinkedList_push(OrderedLinkedList_Object* self, PyObject* args);
PyObject* wasp__ollist__OrderedLinkedList_pull(OrderedLinkedList_Object* self, PyObject* args);
PyObject* wasp__ollist__OrderedLinkedList_next(OrderedLinkedList_Object* self, PyObject* args);

#endif // __WASP_C_EXTENSIONS__OLLIST_OLLIST_WRAPPER_H__
