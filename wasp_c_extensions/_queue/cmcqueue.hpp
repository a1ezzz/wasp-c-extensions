// wasp_c_extensions/_queue/cmcqueue.hpp
//
//Copyright (C) 2020 the wasp-c-extensions authors and contributors
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

#ifndef __WASP_C_EXTENSIONS__QUEUE_CMCQUEUE_HPP__
#define __WASP_C_EXTENSIONS__QUEUE_CMCQUEUE_HPP__

#include <atomic>

enum WServiceMsg {
    WSERVICE_MSG_USERS_PAYLOAD = 0,
    WSERVICE_MSG_CMD_SUBSCRIPTION = 1,
    WSERVICE_MSG_CMD_UNSUBSCRIPTION = 2
};

class WCMCQueue;
class WCMCQueueItem;

class WCMCQueue {
    // somewhere should be a note that this simple implementation may lead to OOM when items are pushed faster then popped

    // Ring buffer? -- that should be improved implementation with extra locks

    std::atomic<WCMCQueueItem*> janitor_tail;  // how to clean up?
    std::atomic<WCMCQueueItem*> head;  // test_and_set
    std::atomic<size_t> items_cnt;  // increment by one; decrement by one

    // janitor's stuff
    std::atomic<size_t> last_subscribes;  // increment by one; decrement by one

    void swipe();

    public:

        virtual ~WCMCQueue();

        void push(WCMCQueueItem* next_item);  // just set new item to the head with subscribers counter
        // equal to zero
        // what to do if there are no subscribers? -- flood may happen -- it's not a problem for non-ring-buffer implementation
        // notification solution:
        //  - call a virtual method like notify. A basic implementation does nothing.
        //  - More complex and multi-threaded implementation may be like this
        //    -- There is a single "object" per thread
        //    -- This thread-object waits for a specific event that is linked to a specific queue (multiple queues?)
        //    -- A thread that waits do nothing
        //    -- Other threads do their work
        //    -- There is a set of thread-objects that wait for a specific queue
        //    -- Queue may notify this set (and all the thread-objects about new item)
        //  - Single-threaded implementation (like asyncio) may be implemented side-by-side with a multi-threaded one
        void pop(WCMCQueueItem* prev_node);  // try to get the next node
        // if there is a next node:
        //    - increment subscriber's counter
        //    - get a payload
        //    - if subscriber's count equal to the last_subscribes -- clean up:
        //       - for non-user_payload: do the math (increase/decrease subscriber's counter)
        //       - switch janitor_tail
        //    - if it is a user's payload, return it
        // if there are no next node -- quit? and do not wait?
        // notification? how to wait for the next item?

        size_t length();  // approximate value
        size_t subscribers();  // approximate value

        WCMCQueueItem* subscribe(); // set next queue item with subscribe command
        void unsubscribe(WCMCQueueItem* prev_node); // set next queue item with unsubscribe command
};


class WCMCQueueItem {
    std::atomic<WCMCQueueItem*> next_item;  // test_and_set

    void* payload;

    public:
        virtual ~WCMCQueueItem();

        bool append(void* payload); // try to append next item with payload and return true if item inserted
        // false -- otherwise
        WCMCQueueItem* next(); // return next_item;
};

/// ??? subscribed msg
/// ??? unsubscribed msg ???



/*
static PyObject* WMultipleConsumersQueue_Object_subscribe(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_unsubscribe(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_push(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_pop(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_has(WMultipleConsumersQueue_Object* self, PyObject* args);
static int WMultipleConsumersQueue_Object_args_index(
    WMultipleConsumersQueue_Object* self, PyObject* args, Py_ssize_t* args_index, bool* valid_index
);
static PyObject* WMultipleConsumersQueue_Object_count(WMultipleConsumersQueue_Object* self, PyObject* args);
static PyObject* WMultipleConsumersQueue_Object_clean(
*/

//#include <Python.h>
//#include <stddef.h>

//#include "common.h"

//extern PyTypeObject WMultipleConsumersQueue_Type;
//
//typedef struct {
//	PyObject_HEAD
//	PyObject* __callback;
//	Py_ssize_t __subscribers;
//	PyLongObject* __index_delta;
//	PyListObject* __queue;
//	PyObject* __weakreflist;
//} WMultipleConsumersQueue_Object;

#endif // __WASP_C_EXTENSIONS__QUEUE_CMCQUEUE_HPP__
