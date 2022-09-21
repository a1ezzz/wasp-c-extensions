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
#include <iostream>
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

        void dump_items_to_clog();
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

        virtual void destroyable();

        static void heap_destroy_fn(PointerDestructor*);

        static void stack_destroy_fn(PointerDestructor*);

        virtual const char* gc_item_id();
};

};  // namespace wasp::cgc

#endif  //  __WASP_C_EXTENSIONS__CGC_CGC_HPP__
