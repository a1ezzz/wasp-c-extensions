// wasp_c_extensions/module_common.h
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

#ifndef __WASP_C_EXTENSIONS__MODULE_COMMON_H__
#define __WASP_C_EXTENSIONS__MODULE_COMMON_H__

#include <Python.h>

#include "common.h"

static PyObject* add_type_to_module(PyObject* module, PyTypeObject* type, const char* type_name){
	if (PyType_Ready(type) < 0){
		__WASP_DEBUG__("Unable to prepare \"%s\"", type_name);
		return NULL;
	}
	__WASP_DEBUG__("Type \"%s\" prepared", type_name);
	Py_INCREF(type);
	PyModule_AddObject(module, type_name, (PyObject*) type);
    __WASP_DEBUG__("Type \"%s\" was linked", type_name);
    return module;
}

#endif // __WASP_C_EXTENSIONS__MODULE_COMMON_H__
