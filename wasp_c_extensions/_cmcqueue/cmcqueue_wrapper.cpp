// wasp_c_extensions/_queue/cmcqueue_wrapper.cpp
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

extern "C" {
#include "cmcqueue_wrapper.h"
}

#include "cmcqueue.hpp"

using namespace wasp::queue;

inline static void cmcqueue_item_cleanup(QueueItem* item)
{
    Py_DECREF(item->payload);
}

typedef CMCQueue<wasp::queue::StretchedBuffer, cmcqueue_item_cleanup> queue_type;

PyObject* wasp__queue__CMCQueue_new(PyTypeObject* type, PyObject* args, PyObject* kwargs){
    __WASP_DEBUG__("Allocation of \"" __STR_CMCQUEUE_NAME__ "\" object");

    CMCQueue_Object* self = (CMCQueue_Object *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }
    self->__queue = new queue_type();

    __WASP_DEBUG__("Object \""  __STR_CMCQUEUE_NAME__ "\" was allocated");
    return (PyObject *) self;
}

void wasp__queue__CMCQueue_dealloc(CMCQueue_Object* self){
    __WASP_DEBUG__("Deallocation of \"" __STR_CMCQUEUE_NAME__ "\" object");

    if (self->__weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }

    delete (static_cast<queue_type*>(self->__queue));
    Py_TYPE(self)->tp_free((PyObject *) self);

    __WASP_DEBUG__("Object \"" __STR_CMCQUEUE_NAME__ "\" was deallocated");
}

PyObject* wasp__queue__CMCQueue_push(CMCQueue_Object* self, PyObject* args){
    __WASP_DEBUG__("Push payload to \"" __STR_CMCQUEUE_NAME__ "\" instance");;

    PyObject* msg = NULL;
    if (! PyArg_ParseTuple(args, "O", &msg)){
        PyErr_SetString(PyExc_ValueError, "Message parsing error");
        return NULL;
    }

    Py_INCREF(msg);
    (static_cast<queue_type*>(self->__queue))->push(msg);
	Py_RETURN_NONE;
}
