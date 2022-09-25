// wasp_c_extensions/_pqueue/pqueue.hpp
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

#ifndef __WASP_C_EXTENSIONS__PQUEUE_PQUEUE_HPP__
#define __WASP_C_EXTENSIONS__PQUEUE_PQUEUE_HPP__

#include <atomic>
#include <cstddef>
#include <utility>

#include "_cgc/cgc.hpp"
#include "_cgc/smart_ptr.hpp"

namespace wasp::pqueue {

typedef long int item_priority;

class QueueItem;

class QueueItem:
    public wasp::cgc::ConcurrentGCItem
{
    QueueItem(
        void (*destroy_fn)(PointerDestructor*),
        const item_priority priority,
        const void* payload,
        wasp::cgc::CGCSmartPointer<QueueItem>* smart_pointer
    ); // private constructor forbids creation on stack

    public:
        virtual ~QueueItem();

        const item_priority priority;
        const void* payload;

        std::atomic<QueueItem*> next_item;   // TODO: check if it is possible to hide
        std::atomic<bool> read_flag;  // whether this node has been read or not
        wasp::cgc::CGCSmartPointer<QueueItem>* smart_pointer;

        static QueueItem* create(
            wasp::cgc::ConcurrentGarbageCollector* gc, const item_priority priority, const void* payload
        );

        void gc_item_id(std::ostream&);
};

class PriorityQueue{

    typedef std::pair<QueueItem*,QueueItem*> priority_pair;

    wasp::cgc::ConcurrentGarbageCollector* gc;
    std::atomic<wasp::cgc::CGCSmartPointer<QueueItem>*> head;

    std::atomic<bool> cleanup_running;  // TODO: check if this block may be safely bypassed

    priority_pair search_next(QueueItem*, const item_priority);

    bool cleanup_head();
    void cleanup(bool call_gc);  // TODO: this require that all parallel requests are completed which is not
    // quite concurrent think of a smarted cleaning like to switch head earlier (may be
    // the CGCSmartPointer::block_mode method will help)

    QueueItem* acquire_head_for_push(QueueItem*);

    public:
        PriorityQueue(wasp::cgc::ConcurrentGarbageCollector*);
        virtual ~PriorityQueue();

        virtual void push(const item_priority priority, const void* payload);

        virtual const item_priority next(item_priority default_value);

        virtual const void* pull(); // do some gc!
};

};  // namespace wasp::pqueue

#endif  //  __WASP_C_EXTENSIONS__PQUEUE_PQUEUE_HPP__
