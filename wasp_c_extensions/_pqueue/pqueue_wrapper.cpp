// wasp_c_extensions/_pqueue/pqueue_wrapper.cpp
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

extern "C" {
#include "pqueue_wrapper.h"
}

#include "pqueue.hpp"

using namespace wasp::pqueue;

PyObject* wasp__pqueue__cgc_module = NULL;

PyObject* wasp__pqueue__PriorityQueue_new(PyTypeObject* type, PyObject* args, PyObject* kwargs){
    PriorityQueue_Object* self = (PriorityQueue_Object *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }

    self->__queue = new PriorityQueue();

    __WASP_DEBUG__("PriorityQueue_Object object was allocated");
    return (PyObject *) self;
}

void wasp__pqueue__PriorityQueue_dealloc(PriorityQueue_Object* self){

    if (self->__queue){
        delete (static_cast<PriorityQueue*>(self->__queue));
        self->__queue = NULL;
    }

    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyObject* wasp__pqueue__PriorityQueue_push(PriorityQueue_Object* self, PyObject* args)
{
    item_priority priority = 0;
    PyObject* payload = NULL;
    if (! PyArg_ParseTuple(args, "lO", &priority, &payload)){  // "O"-values do not increment ref. counter
        PyErr_SetString(PyExc_ValueError, "Callback parsing error");
        return NULL;
    }

    static_cast<PriorityQueue*>(self->__queue)->push(new QueueItem(priority, payload));

    Py_RETURN_TRUE;
}

PyObject* wasp__pqueue__PriorityQueue_pull(PriorityQueue_Object* self, PyObject* args)
{
    Py_RETURN_TRUE;
}

PyObject* wasp__pqueue__PriorityQueue_next(PriorityQueue_Object* self, PyObject* args)
{
    Py_RETURN_TRUE;
}
