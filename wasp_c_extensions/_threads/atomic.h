// wasp_c_extensions/_threads/atomic.h
//
//Copyright (C) 2018 the wasp-c-extensions authors and contributors
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

#ifndef __WASP_C_EXTENSIONS__THREADS_ATOMIC_H__
#define __WASP_C_EXTENSIONS__THREADS_ATOMIC_H__

#include <Python.h>
#include <stddef.h>

#include "_common/common.h"

PyObject* __py_int_add_fn__;

PyTypeObject WAtomicCounter_Type;

typedef struct {
	PyObject_HEAD
	PyLongObject* __int_value;
	PyObject *__weakreflist;
} WAtomicCounter_Object;

#endif // __WASP_C_EXTENSIONS__THREADS_ATOMIC_H__
