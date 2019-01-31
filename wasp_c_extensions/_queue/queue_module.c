// wasp_c_extensions/_queue/queue_module.c
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

#include <Python.h>

#include "_common/common.h"
#include "mcqueue.h"

static struct PyModuleDef queue_module = {
	PyModuleDef_HEAD_INIT,
	.m_name = __STR_PACKAGE_NAME__"."__STR_QUEUE_MODULE_NAME__,
	.m_doc = "",
//	.m_doc =
//		"This module "__STR_PACKAGE_NAME__"."__STR_THREADS_MODULE_NAME__" contains following classes:\n"
//		__STR_ATOMIC_COUNTER_NAME__" class that may be used as a counter which modification via "
//		__STR_ATOMIC_COUNTER_NAME__".increase method which call is atomic (is thread safe)\n"
//		__STR_PTHREAD_EVENT_NAME__" class that behave the same way as threading.Event does, but runs faster "
//		"because of implementation with phtread library."
//	,
	.m_size = -1,
};

PyMODINIT_FUNC __PYINIT_QUEUE_MAIN_FN__ (void) {

	__WASP_DEBUG_PRINTF__(
		"Module \""__STR_PACKAGE_NAME__"."__STR_QUEUE_MODULE_NAME__"\" initialization call"
	);

	PyObject *m = NULL;

	if (PyType_Ready(&WMultipleConsumersQueue_Type) < 0){
		__WASP_DEBUG_PRINTF__("Unable to prepare \""__STR_MCQUEUE_NAME__"\"");
		return NULL;
	}
	__WASP_DEBUG_PRINTF__("Type \""__STR_MCQUEUE_NAME__"\" was initialized");

	m = PyModule_Create(&queue_module);
	if (m == NULL)
		return NULL;

	__WASP_DEBUG_PRINTF__("Module \""__STR_PACKAGE_NAME__"."__STR_QUEUE_MODULE_NAME__"\" was created");

	Py_INCREF(&WMultipleConsumersQueue_Type);
	PyModule_AddObject(m, __STR_MCQUEUE_NAME__, (PyObject*) &WMultipleConsumersQueue_Type);
        __WASP_DEBUG_PRINTF__("Type \""__STR_QUEUE_MODULE_NAME__"\" was linked");

	return m;
}
