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
    __WASP_DEBUG__("Release a payload");
    Py_DECREF(item->payload);
}

// CMCQueue functions

PyObject* wasp__cmcqueue__CMCQueue_new(PyTypeObject* type, PyObject* args, PyObject* kwargs){
    __WASP_DEBUG__("Allocation of \"" __STR_CMCQUEUE_NAME__ "\" object");

    CMCQueue_Object* self = (CMCQueue_Object *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }
    self->__queue = dynamic_cast<ICMCQueue*>(new CMCQueue<wasp::queue::StretchedBuffer, cmcqueue_item_cleanup>());

    __WASP_DEBUG__("Object \""  __STR_CMCQUEUE_NAME__ "\" was allocated");
    return (PyObject *) self;
}

void wasp__cmcqueue__CMCQueue_dealloc(CMCQueue_Object* self){
    __WASP_DEBUG__("Deallocation of object");

    if (self->__weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }

    delete (static_cast<ICMCQueue*>(self->__queue));
    Py_TYPE(self)->tp_free((PyObject *) self);

    __WASP_DEBUG__("Object was deallocated");
}

PyObject* wasp__cmcqueue__CMCQueue_push(CMCQueue_Object* self, PyObject* args){
    __WASP_DEBUG__("Push payload to \"" __STR_CMCQUEUE_NAME__ "\" instance");;

    PyObject* msg = NULL;
    if (! PyArg_ParseTuple(args, "O", &msg)){  // "O"-values do not increment ref. counter
        PyErr_SetString(PyExc_ValueError, "Message parsing error");
        return NULL;
    }

    Py_INCREF(msg);
    (static_cast<ICMCQueue*>(self->__queue))->push(msg);
	Py_RETURN_NONE;
}

PyObject* wasp__cmcqueue__CMCQueue_subscribe(CMCQueue_Object* self, PyObject* args){

    __WASP_DEBUG__("Subscribing to \"" __STR_CMCQUEUE_NAME__ "\" instance")

    CMCQueueItem_Object* queue_item = (CMCQueueItem_Object*) wasp__cmcqueue__CMCQueueItem_new(
        wasp__cmcqueue__CMCQueueItem_type(), NULL, NULL
    );

    Py_INCREF(self);

    queue_item->__py_queue = self;
    queue_item->__last_item = (static_cast<ICMCQueue*>(self->__queue))->subscribe();

    return (PyObject*) queue_item;
}

PyObject* wasp__cmcqueue__CMCQueue_messages(CMCQueue_Object* self, PyObject* args){
    size_t messages = static_cast<ICMCQueue*>(self->__queue)->messages();
    return PyLong_FromSize_t(messages);
}

// CMCQueueItem functions

static int wasp__cmcqueue__CMCQueueItem_unsubscribe_impl(CMCQueueItem_Object* self);

PyObject* wasp__cmcqueue__CMCQueueItem_new(PyTypeObject* type, PyObject* args, PyObject* kwargs){
    __WASP_DEBUG__("Allocation of \"" __STR_CMCQUEUE_ITEM_NAME__ "\" object");

    CMCQueueItem_Object* self = (CMCQueueItem_Object *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }
    self->__py_queue = NULL;
    self->__last_item = NULL;

    __WASP_DEBUG__("Object \""  __STR_CMCQUEUE_ITEM_NAME__ "\" was allocated");
    return (PyObject *) self;
}

int wasp__cmcqueue__CMCQueueItem_init(CMCQueueItem_Object *self, PyObject *args, PyObject *kwargs){
    PyErr_SetString(
        PyExc_RuntimeError,
        "The \"" __STR_CMCQUEUE_ITEM_NAME__ "\" object shouldn't be created directly. Please call for a subscription"
    );
    return -1;
}

void wasp__cmcqueue__CMCQueueItem_dealloc(CMCQueueItem_Object* self){
    __WASP_DEBUG__("Deallocation of a queue item object");
    wasp__cmcqueue__CMCQueueItem_unsubscribe_impl(self);
    Py_TYPE(self)->tp_free((PyObject *) self);
    __WASP_DEBUG__("Object \"" __STR_CMCQUEUE_ITEM_NAME__ "\" was deallocated");
}

static int wasp__cmcqueue__CMCQueueItem_unsubscribe_impl(CMCQueueItem_Object* self){
    __WASP_DEBUG__("Unsubscribe queue item (implementation)");

    CMCQueue_Object* py_queue = (CMCQueue_Object*) self->__py_queue;

    if (py_queue){
        if (self->__last_item){
            static_cast<ICMCQueue*>(py_queue->__queue)->unsubscribe(
               static_cast<const QueueItem*>(self->__last_item)
            );
            self->__last_item = NULL;
        }

        Py_DECREF(py_queue);
        self->__py_queue = NULL;
        return 0;
    }

    return -1;
}

PyObject* wasp__cmcqueue__CMCQueueItem_unsubscribe(CMCQueueItem_Object* self, PyObject* args){
    __WASP_DEBUG__("Unsubscribe queue item");

    if(! wasp__cmcqueue__CMCQueueItem_unsubscribe_impl(self)){
        Py_RETURN_NONE;
    }

    PyErr_SetString(PyExc_RuntimeError, "Queue item may be unsubscribed only once");
    return NULL;
}

PyObject* wasp__cmcqueue__CMCQueueItem_pull(CMCQueueItem_Object* self, PyObject* args){
    CMCQueue_Object* py_queue = (CMCQueue_Object*) self->__py_queue;
    const QueueItem *last_item = (const QueueItem*) self->__last_item, *next_item = NULL;

    if ((! py_queue) || (! last_item)){
        PyErr_SetString(PyExc_RuntimeError, "Unable to pull unsubscribed object");
        return NULL;
    }

    next_item = static_cast<ICMCQueue*>(py_queue->__queue)->pull(last_item);
    if (next_item == last_item){
        Py_RETURN_NONE;  // TODO: or raise something
    }

    self->__last_item = next_item;

    if (next_item){
        if (next_item->payload){  // not the pointer switch
            Py_INCREF(next_item->payload);  // TODO: double check -- payload is 'increfed' already
            return (PyObject*) next_item->payload;
        }
    }

    Py_RETURN_NONE;
}

PyObject* wasp__cmcqueue__CMCQueueItem_has_next(CMCQueueItem_Object* self, PyObject* args){
    CMCQueue_Object* py_queue = (CMCQueue_Object*) self->__py_queue;
    const QueueItem *last_item = (const QueueItem*) self->__last_item;

    if ((! py_queue) || (! last_item)){
        PyErr_SetString(PyExc_RuntimeError, "Unable to pull unsubscribed object");
        return NULL;
    }

    if (static_cast<ICMCQueue*>(py_queue->__queue)->has_next(last_item)){
        Py_RETURN_TRUE;
    }

    Py_RETURN_FALSE;
}
