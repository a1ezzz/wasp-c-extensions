// wasp_c_extensions/_ev_loop/ev_loop_module.c
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

// TODO: #include <stddef.h>

#include "module_common.h"

#include "ev_loop_wrapper.h"

static void wasp__ev_loop__module_free(void*);

static PyMethodDef EventLoop_methods[] = {

    {
        "notify", (PyCFunction) wasp__ev_loop__EventLoop_notify, METH_VARARGS,
        "\"Notify\" description.\n"
    },

    {
        "process_event", (PyCFunction) wasp__ev_loop__EventLoop_process_event, METH_NOARGS,
        "\"Process event\" description.\n"
    },

    {
        "start_loop", (PyCFunction) wasp__ev_loop__EventLoop_start_loop, METH_NOARGS,
        "\"Start loop\" description.\n"
    },

    {
        "stop_loop", (PyCFunction) wasp__ev_loop__EventLoop_stop_loop, METH_NOARGS,
        "\"Stop loop\" description.\n"
    },

    {NULL}
};

static PyTypeObject EventLoop_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = __STR_PACKAGE_NAME__ "." __STR_MODULE_NAME__ "." __STR_EVENT_LOOP_NAME__,
    .tp_basicsize = sizeof(EventLoop_Type),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Simple description placement",

    .tp_new = wasp__ev_loop__EventLoop_new,
    .tp_init = (initproc) wasp__ev_loop__EventLoop_init,
    .tp_dealloc = (destructor) wasp__ev_loop__EventLoop_dealloc,
    .tp_methods = EventLoop_methods,
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    .m_name = __STR_PACKAGE_NAME__ "." __STR_MODULE_NAME__,
    .m_doc = "This is the \"" __STR_PACKAGE_NAME__ "." __STR_MODULE_NAME__"\" module",
    .m_size = -1,
    .m_free = wasp__ev_loop__module_free,
};

PyMODINIT_FUNC __PYINIT_MAIN_FN__ (void) {

    __WASP_DEBUG__("Module is about to initialize");

    wasp__ev_loop__cmcqueue_module = PyImport_ImportModule(  // new ref or NULL with exception
        __STR_PACKAGE_NAME__ "." __STR_FN_CALL__(__CMCMODULE_NAME__)
    );

    if (! wasp__ev_loop__cmcqueue_module){
        return NULL;
    }

    wasp__ev_loop__cmcqueue_type = PyObject_GetAttrString(  // new ref
        wasp__ev_loop__cmcqueue_module, __STR_FN_CALL__(__CMCQUEUE_NAME__)
    );

    if (! wasp__ev_loop__cmcqueue_type){
        PyErr_SetString(PyExc_RuntimeError, "Required type isn't available");
        Py_DECREF(wasp__ev_loop__cmcqueue_module);
        wasp__ev_loop__cmcqueue_module = NULL;
        return NULL;
    }

    PyObject* m = PyModule_Create(&module);
    if (m == NULL)
        return NULL;

    if (! add_type_to_module(m, &EventLoop_Type, __STR_EVENT_LOOP_NAME__)){
        return NULL;
    }

    __WASP_DEBUG__("Module was created");

    return m;
}

void wasp__ev_loop__module_free(void* m){
    if (wasp__ev_loop__cmcqueue_type){
        Py_DECREF(wasp__ev_loop__cmcqueue_type);
        wasp__ev_loop__cmcqueue_type = NULL;
    }

    if (wasp__ev_loop__cmcqueue_module){
        Py_DECREF(wasp__ev_loop__cmcqueue_module);
        wasp__ev_loop__cmcqueue_module = NULL;
    }
}
