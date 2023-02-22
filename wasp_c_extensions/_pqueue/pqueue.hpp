// wasp_c_extensions/_pqueue/pqueue.hpp
//
//Copyright (C) 2023 the wasp-c-extensions authors and contributors
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

class QueueItem;

typedef long int item_priority;
typedef wasp::cgc::CGCSmartPointer<QueueItem> smart_item_pointer;

class QueueItem:
    public wasp::cgc::ConcurrentGCItem
{
    QueueItem(
        void (*destroy_fn)(PointerDestructor*),
        const item_priority priority,
        const void* payload
    ); // private constructor forbids creation on stack

    public:
        virtual ~QueueItem();

        const item_priority priority;
        const void* payload;

        std::atomic<bool> read_flag;  // whether this node has been read or not. TODO: check if it is possible to hide
        std::atomic<smart_item_pointer*> sorted_next;   // TODO: check if it is possible to hide
        std::atomic<QueueItem*> raw_next;  // TODO: check if it is possible to hide

        static QueueItem* create(
            wasp::cgc::ConcurrentGarbageCollector* gc, const item_priority priority, const void* payload
        );

        void gc_item_id(std::ostream&);
};

class PriorityQueue{

    wasp::cgc::ConcurrentGarbageCollector* gc;
    std::atomic_flag is_merge_and_cleanup_running;
    std::atomic<QueueItem*> raw_head;  // unsorted items

    wasp::cgc::SmartDualPointer<QueueItem> sorted_head; // TODO: rename to sorted_head

    std::atomic<size_t> cache_size_counter;
    std::atomic<size_t> queue_size_counter;

    QueueItem* detach_raw_head();  // used by write_flush. Under "lock"!
    void merge_raw_head(QueueItem*);  // used by write_flush. Under "lock"!
    QueueItem* sort_raw_items(QueueItem*);  // used by write_flush

    void cleanup(bool run_gc=true);

    void read_flush();  // mark everything as read

    public:
        PriorityQueue(wasp::cgc::ConcurrentGarbageCollector*);  // TODO: add a reverse order!
        virtual ~PriorityQueue();

        virtual void push(const item_priority priority, const void* payload);

        virtual const void* pull(const bool probe = false);

        bool write_flush();

        size_t cache_size();

        size_t queue_size();

        void dump(); // TODO: debug only
};

};  // namespace wasp::pqueue

#endif  //  __WASP_C_EXTENSIONS__PQUEUE_PQUEUE_HPP__
