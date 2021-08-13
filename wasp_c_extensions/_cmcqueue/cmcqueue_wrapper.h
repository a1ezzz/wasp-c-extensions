// wasp_c_extensions/_queue/cmcqueue_wrapper.h
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

#ifndef __WASP_C_EXTENSIONS__CMCQUEUE_CMCQUEUE_WRAPPER_H__
#define __WASP_C_EXTENSIONS__CMCQUEUE_CMCQUEUE_WRAPPER_H__

#include <Python.h>

#include "common.h"

#define __STR_CMCQUEUE_NAME__ __STR_FN_CALL__(__CMCQUEUE_NAME__)
#define __STR_CMCQUEUE_ITEM_NAME__ __STR_FN_CALL__(__CMCQUEUE_ITEM_NAME__)

typedef struct {
	PyObject_HEAD
	void* __queue;
	PyObject* __weakreflist;
} CMCQueue_Object;

typedef struct {
	PyObject_HEAD
	void* __py_queue;
	const void* __last_item;
} CMCQueueItem_Object;

PyObject* wasp__cmcqueue__CMCQueue_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
void wasp__cmcqueue__CMCQueue_dealloc(CMCQueue_Object* self);

PyObject* wasp__cmcqueue__CMCQueue_push(CMCQueue_Object* self, PyObject* args);
PyObject* wasp__cmcqueue__CMCQueue_subscribe(CMCQueue_Object* self, PyObject* args);
PyObject* wasp__cmcqueue__CMCQueue_messages(CMCQueue_Object* self, PyObject* args);

PyTypeObject* wasp__cmcqueue__CMCQueueItem_type();

PyObject* wasp__cmcqueue__CMCQueueItem_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
int wasp__cmcqueue__CMCQueueItem_init(CMCQueueItem_Object *self, PyObject *args, PyObject *kwargs);
void wasp__cmcqueue__CMCQueueItem_dealloc(CMCQueueItem_Object* self);

PyObject* wasp__cmcqueue__CMCQueueItem_unsubscribe(CMCQueueItem_Object* self, PyObject* args);
PyObject* wasp__cmcqueue__CMCQueueItem_pull(CMCQueueItem_Object* self, PyObject* args);
PyObject* wasp__cmcqueue__CMCQueueItem_has_next(CMCQueueItem_Object* self, PyObject* args);

#endif // __WASP_C_EXTENSIONS__CMCQUEUE_CMCQUEUE_WRAPPER_H__
