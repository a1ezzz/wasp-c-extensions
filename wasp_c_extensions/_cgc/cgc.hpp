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

class ConcurrentGCItem{
    public:
        ConcurrentGCItem(void (*destroy_fn)(ConcurrentGCItem*));
        virtual ~ConcurrentGCItem();

        void (*const destroy_fn)(ConcurrentGCItem*);

        std::atomic<bool> gc_ready;
        std::atomic<ConcurrentGCItem*> next;

        static void destroy(ConcurrentGCItem*);
};

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

};  // namespace wasp::cgc

#endif  //  __WASP_C_EXTENSIONS__CGC_CGC_HPP__
