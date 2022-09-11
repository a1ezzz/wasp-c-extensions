// wasp_c_extensions/_cgc/cgc.hpp
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

#ifndef __WASP_C_EXTENSIONS__CGC_CGC_HPP__
#define __WASP_C_EXTENSIONS__CGC_CGC_HPP__

#include <atomic>
#include <cassert>
#include <cstddef>

namespace wasp::cgc {

class NullPointerException{};

class InvalidItemState{};

class ConcurrentGCItem;

class ConcurrentGarbageCollector{

    std::atomic<ConcurrentGCItem*> head;
    std::atomic<size_t> parallel_gc;
    std::atomic<size_t> count;

    bool __push(ConcurrentGCItem*, ConcurrentGCItem*);

    ConcurrentGCItem* detach_head();

    public:
        ConcurrentGarbageCollector();
        virtual ~ConcurrentGarbageCollector();

        size_t items();

        void push(ConcurrentGCItem*);

        void collect();  // try to clear everything from this GC. This method should not called often
};

class PointerDestructor{
    void (*const destroy_fn)(PointerDestructor*);

    public:

        PointerDestructor(void (*destroy_fn)(PointerDestructor*));
        virtual ~PointerDestructor();

        virtual void destroyable() = 0;

        static void destroy(PointerDestructor*);
};

class ConcurrentGCItem:
    public PointerDestructor
{
    volatile bool gc_ready_flag;  // There is only one lazy switch false->true, so it may not be atomic
    std::atomic<ConcurrentGCItem*> gc_next;

    friend ConcurrentGarbageCollector;

    public:
        ConcurrentGCItem(void (*destroy_fn)(PointerDestructor*));
        virtual ~ConcurrentGCItem();

        void destroyable();
};

class ResourceSmartLock{

    std::atomic<bool> is_dead;                     // marks this resource as unavailable
    std::atomic<size_t> usage_counter;             // this counter shows how many pending "releases" there are.
    // This counter is for managing acquire-release concurrency
    std::atomic<size_t> concurrency_call_counter;  // this counter show how many calls to the "acquire" method
    // are at the moment. This counter is for managing acquire-reset concurrency
    std::atomic<bool> concurrency_liveness_flag;   // This flag indicate the "acquire" method that the "release" is
    // on the fly

    public:
        ResourceSmartLock();
        virtual ~ResourceSmartLock();

        bool able_to_reset();  // check that a resource is capable to be replaced

        bool reset();  // return true, if internal counter/flags are reset. This indicated that resource may be changed
        // multiple concurrent requests to this method will make errors

        bool acquire();  // acknowledge that a shared resource is requested (returns true if the resource is available)

        bool release();  // acknowledge that a shared resource is no longer in use. For every successful "acquire"
        // a single release must be called. (returns true if the resource is ready to be destroyed)
};

class SmartPointer
{

    ResourceSmartLock pointer_lock;
    std::atomic<PointerDestructor*> pointer;
    std::atomic<PointerDestructor*> zombie_pointer;  // this pointer protect the pointer_lock.reset method
    // from being called concurrent

    public:
        SmartPointer(PointerDestructor*);
        virtual ~SmartPointer();

        virtual PointerDestructor* acquire();
        virtual void release();

        bool replace(PointerDestructor* new_ptr);
};

};  // namespace wasp::cgc

#endif  //  __WASP_C_EXTENSIONS__CGC_CGC_HPP__
