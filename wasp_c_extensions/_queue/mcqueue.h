// wasp_c_extensions/_queue/mcqueue.h
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

#ifndef __WASP_C_EXTENSIONS__QUEUE_MCQUEUE_H__
#define __WASP_C_EXTENSIONS__QUEUE_MCQUEUE_H__

#include <Python.h>
#include <stddef.h>

#include "common.h"

extern PyTypeObject WMultipleConsumersQueue_Type;

typedef struct {
	PyObject_HEAD
	PyObject* __callback;
	Py_ssize_t __subscribers;
	PyLongObject* __index_delta;
	PyListObject* __queue;
	PyObject* __weakreflist;
} WMultipleConsumersQueue_Object;

#endif // __WASP_C_EXTENSIONS__QUEUE_MCQUEUE_H__
