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

#include "common.h"
#include "atomic.h"
#include "event.h"
#include "module_functions.h"

static PyMethodDef threads_methods[] = {

	{
		"awareness_wait", (PyCFunction) awareness_wait, METH_VARARGS | METH_KEYWORDS,
		"Synchronize event's state and wait for this event to come. At first event is cleared and then if "
		"a \"sync_fn\" function returns True event will be set. If not - event will be awaited\n"
		"\n"
		":param event: an event to synchronize with and to wait for\n"
		":type event: Event | "__STR_PTHREAD_EVENT_NAME__"\n"
		"\n"
		":param sync_fn: a callable object that must return 'bool' object. This object may help to "
		"synchronize the event\n"
		":type sync_fn: callable\n"
		"\n"
		":param timeout: if defined then this is a time in seconds during which an event will be awaited "
		"(default is no timeout)\n"
		":type timeout: int | float | None\n"
		"\n"
		":rtype: bool\n"
	},

	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef threads_module = {
	PyModuleDef_HEAD_INIT,
	.m_name = __STR_PACKAGE_NAME__"."__STR_THREADS_MODULE_NAME__,
	.m_doc =
		"This module "__STR_PACKAGE_NAME__"."__STR_THREADS_MODULE_NAME__" contains following classes:\n"
		__STR_ATOMIC_COUNTER_NAME__" class that may be used as a counter which modification via "
		__STR_ATOMIC_COUNTER_NAME__".increase method which call is atomic (is thread safe)\n"
		__STR_PTHREAD_EVENT_NAME__" class that behave the same way as threading.Event does, but runs faster "
		"because of implementation with phtread library.\n"
		"\"awareness_wait\" function that may synchronize event with some external state"
	,
	.m_size = -1,
	threads_methods
};

PyMODINIT_FUNC __PYINIT_THREADS_MAIN_FN__ (void) {

	__WASP_DEBUG_PRINTF__(
		"Module \""__STR_PACKAGE_NAME__"."__STR_THREADS_MODULE_NAME__"\" initialization call"
	);

	PyObject *m;

    __zero = (PyLongObject*) PyLong_FromLong(0);

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

    PyModule_AddIntConstant(m, WASP_ATOMIC_LT_TEST_NAME, Py_LT);
    PyModule_AddIntConstant(m, WASP_ATOMIC_LE_TEST_NAME, Py_LE);
    PyModule_AddIntConstant(m, WASP_ATOMIC_EQ_TEST_NAME, Py_EQ);
    PyModule_AddIntConstant(m, WASP_ATOMIC_NE_TEST_NAME, Py_NE);
    PyModule_AddIntConstant(m, WASP_ATOMIC_GT_TEST_NAME, Py_GT);
    PyModule_AddIntConstant(m, WASP_ATOMIC_GE_TEST_NAME, Py_GE);

	Py_INCREF(&WAtomicCounter_Type);
	PyModule_AddObject(m, __STR_ATOMIC_COUNTER_NAME__, (PyObject*) &WAtomicCounter_Type);
        __WASP_DEBUG_PRINTF__("Type \""__STR_ATOMIC_COUNTER_NAME__"\" was linked");

	Py_INCREF(&WPThreadEvent_Type);
	PyModule_AddObject(m, __STR_PTHREAD_EVENT_NAME__, (PyObject*) &WPThreadEvent_Type);
        __WASP_DEBUG_PRINTF__("Type \""__STR_PTHREAD_EVENT_NAME__"\" was linked");

	return m;
}
