// wasp_c_extensions/_cgc/cgc_module.c
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

#include "module_common.h"


static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    .m_name = __STR_PACKAGE_NAME__ "." __STR_MODULE_NAME__,
    .m_doc = "This is the \"" __STR_PACKAGE_NAME__ "." __STR_MODULE_NAME__"\" module",
    .m_size = -1,
};

PyMODINIT_FUNC __PYINIT_MAIN_FN__ (void) {
    // This is a stub method, only for cgc library import

    __WASP_DEBUG__("Module is about to initialize");

    PyObject* m = PyModule_Create(&module);
    if (m == NULL)
        return NULL;

    __WASP_DEBUG__("Module was created");

    return m;
}
