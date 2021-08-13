// wasp_c_extensions/static_functions.h
//
//Copyright (C) 2020 the wasp-c-extensions authors and contributors
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

#ifndef __WASP_C_EXTENSIONS__STATIC_FUNCTIONS_H__
#define __WASP_C_EXTENSIONS__STATIC_FUNCTIONS_H__

static inline int __long_rich_compare_bool(PyLongObject* o1, PyLongObject* o2, int op) {
    PyObject* test_result = NULL;
	int result = -1;

    if (PyLong_Type.tp_richcompare != NULL){
        test_result = PyLong_Type.tp_richcompare((PyObject*) o1, (PyObject*) o2, op);
        result = PyObject_IsTrue(test_result);
    }
    else {
        result = PyObject_RichCompareBool((PyObject*) o1, (PyObject*) o2, op);
    }

    Py_XDECREF(test_result);
    return result;
}

#endif // __WASP_C_EXTENSIONS__THREADS_ATOMIC_H__
