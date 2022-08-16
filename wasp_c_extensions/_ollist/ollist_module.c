// wasp_c_extensions/_ollist/ollist_module.c
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

// TODO: #include <stddef.h>

#include "module_common.h"

#include "ollist_wrapper.h"

static PyMethodDef OrderedLinkedList_methods[] = {

    {
        "push", (PyCFunction) wasp__ollist__OrderedLinkedList_push, METH_VARARGS,
        "\"Push\" description.\n"
    },

    {
        "pull", (PyCFunction) wasp__ollist__OrderedLinkedList_pull, METH_NOARGS,
        "\"Pull\" description.\n"
    },

    {
        "next", (PyCFunction) wasp__ollist__OrderedLinkedList_next, METH_NOARGS,
        "\"Next\" description.\n"
    },

    {NULL}
};

static PyTypeObject OrderedLinkedList_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = __STR_PACKAGE_NAME__ "." __STR_MODULE_NAME__ "." __STR_OLLIST_NAME__,
    .tp_basicsize = sizeof(OrderedLinkedList_Type),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Simple description placement",

    .tp_new = wasp__ollist__OrderedLinkedList_new,
    .tp_dealloc = (destructor) wasp__ollist__OrderedLinkedList_dealloc,
    .tp_methods = OrderedLinkedList_methods,
};


static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    .m_name = __STR_PACKAGE_NAME__ "." __STR_MODULE_NAME__,
    .m_doc = "This is the \"" __STR_PACKAGE_NAME__ "." __STR_MODULE_NAME__"\" module",
    .m_size = -1,
};

PyMODINIT_FUNC __PYINIT_MAIN_FN__ (void) {

    __WASP_DEBUG__("Module is about to initialize");

    PyObject* m = PyModule_Create(&module);
    if (m == NULL)
        return NULL;

    if (! add_type_to_module(m, &OrderedLinkedList_Type, __STR_OLLIST_NAME__)){
        return NULL;
    }

    __WASP_DEBUG__("Module was created");

    return m;
}
