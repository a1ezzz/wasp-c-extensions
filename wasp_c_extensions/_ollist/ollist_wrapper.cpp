// wasp_c_extensions/_ollist/ollist_wrapper.cpp
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
#include "ollist_wrapper.h"
}

#include "ollist.hpp"

using namespace wasp::ollist;

PyObject* wasp__ollist__OrderedLinkedList_new(PyTypeObject* type, PyObject* args, PyObject* kwargs){
    OrderedLinkedList_Object* self = (OrderedLinkedList_Object *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }

    self->__list = new OrderedLinkedList();

    __WASP_DEBUG__("OrderedLinkedList_Object object was allocated");
    return (PyObject *) self;
}

void wasp__ollist__OrderedLinkedList_dealloc(OrderedLinkedList_Object* self){

    if (self->__list){
        delete (static_cast<OrderedLinkedList*>(self->__list));
        self->__list = NULL;
    }

    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyObject* wasp__ollist__OrderedLinkedList_push(OrderedLinkedList_Object* self, PyObject* args)
{
    long priority = 0;
    PyObject* payload = NULL;
    if (! PyArg_ParseTuple(args, "lO", &priority, &payload)){  // "O"-values do not increment ref. counter
        PyErr_SetString(PyExc_ValueError, "Callback parsing error");
        return NULL;
    }

    Py_RETURN_TRUE;
}

PyObject* wasp__ollist__OrderedLinkedList_pull(OrderedLinkedList_Object* self, PyObject* args)
{
    Py_RETURN_TRUE;
}

PyObject* wasp__ollist__OrderedLinkedList_next(OrderedLinkedList_Object* self, PyObject* args)
{
    Py_RETURN_TRUE;
}
