// wasp_c_extensions/_threads/threads_module.c
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

#include <Python.h>

#include "_common/common.h"
#include "atomic.h"
#include "event.h"

static struct PyModuleDef threads_module = {
	PyModuleDef_HEAD_INIT,
	.m_name = __STR_PACKAGE_NAME__"."__STR_THREADS_MODULE_NAME__,
	.m_doc =
		"This module "__STR_PACKAGE_NAME__"."__STR_THREADS_MODULE_NAME__" contains following classes:\n"
		__STR_ATOMIC_COUNTER_NAME__" class that may be used as a counter which modification via "
		__STR_ATOMIC_COUNTER_NAME__".increase method which call is atomic (is thread safe)\n"
		__STR_PTHREAD_EVENT_NAME__" class that behave the same way as threading.Event does, but runs faster "
		"because of implementation with phtread library."
	,
	.m_size = -1,
};

PyMODINIT_FUNC __PYINIT_THREADS_MAIN_FN__ (void) {

	__WASP_DEBUG_PRINTF__(
		"Module \""__STR_PACKAGE_NAME__"."__STR_THREADS_MODULE_NAME__"\" initialization call"
	);

	__py_int_add_fn__ = PyObject_GetAttrString((PyObject*) &PyLong_Type, "__add__");
        if (__py_int_add_fn__ == NULL) {
		return NULL;
        }

        __WASP_DEBUG_PRINTF__("Function \"__py_int_add_fn__\" was initialized");

	PyObject *m;

	if (PyType_Ready(&WAtomicCounter_Type) < 0){
		return NULL;
	}
	__WASP_DEBUG_PRINTF__("Type \""__STR_ATOMIC_COUNTER_NAME__"\" was initialized");

	if (PyType_Ready(&WPThreadEvent_Type) < 0){
		return NULL;
	}
	__WASP_DEBUG_PRINTF__("Type \""__STR_PTHREAD_EVENT_NAME__"\" was initialized");

	m = PyModule_Create(&threads_module);
	if (m == NULL)
		return NULL;

	__WASP_DEBUG_PRINTF__("Module \""__STR_PACKAGE_NAME__"."__STR_THREADS_MODULE_NAME__"\" was created");

	Py_INCREF(&WAtomicCounter_Type);
	PyModule_AddObject(m, __STR_ATOMIC_COUNTER_NAME__, (PyObject*) &WAtomicCounter_Type);
        __WASP_DEBUG_PRINTF__("Type \""__STR_ATOMIC_COUNTER_NAME__"\" was linked");

	Py_INCREF(&WPThreadEvent_Type);
	PyModule_AddObject(m, __STR_PTHREAD_EVENT_NAME__, (PyObject*) &WPThreadEvent_Type);
        __WASP_DEBUG_PRINTF__("Type \""__STR_PTHREAD_EVENT_NAME__"\" was linked");

	return m;
}
