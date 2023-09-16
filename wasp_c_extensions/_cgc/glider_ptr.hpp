// wasp_c_extensions/_cgc/glider_ptr.hpp
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

#ifndef __WASP_C_EXTENSIONS__CGC_GLIDER_PTR_HPP__
#define __WASP_C_EXTENSIONS__CGC_GLIDER_PTR_HPP__

#include "cgc.hpp"
 #include "smart_ptr.hpp"

namespace wasp::cgc {

class GliderPointer;

class GliderPointerNode:
    public ConcurrentGCItem
{

    friend GliderPointer;

    // TODO: think of const constraint for the current_ptr
    ConcurrentGCItem* current_ptr;  // main node value, is not atomic since may not be changed

    std::atomic<GliderPointerNode*> next_node;  // next value ("wave"), managed by the GliderPointer class. shows
    // that cleanup is safe (not the only criteria)

    ShockSemaphore current_ptr_semaphore;  // protects current_ptr
    ShockSemaphore this_semaphore;  // protects this class itself as a part of ConcurrentGCItem

    GliderPointerNode(void (*destroy_fn)(PointerDestructor*), ConcurrentGCItem* ptr);  // hide this in order
    // to restrict inheritance and instantiation

    void acquire_node();  // may be acquired only by the "GliderPointer" class

    public:
        virtual ~GliderPointerNode();

        ConcurrentGCItem* acquire_ptr();
        void release_ptr();

        void release_node();
};

template <typename T>
class GliderContext{

    GliderPointerNode* pointer_node;
    T* casted_ptr;

    private:
        void* operator new(size_t){return NULL;}
        void operator delete(void*){};
        void* operator new[](size_t){return NULL;};

    public:
        GliderContext(GliderPointerNode* ptr):
            pointer_node(ptr),
            casted_ptr(NULL)
        {
            assert(this->pointer_node);
            this->casted_ptr = dynamic_cast<T*>(this->pointer_node->acquire_ptr());
            assert(casted_ptr);
        }

        virtual ~GliderContext(){
            this->casted_ptr = NULL;
            this->pointer_node->release_ptr();
            this->pointer_node->release_node();
        }

        T* operator()(){
            return this->casted_ptr;
        }
};

class GliderPointer{

    ConcurrentGarbageCollector* cgc;

    std::atomic<GliderPointerNode*> tail_ptr;
    std::atomic<GliderPointerNode*> head_ptr;

    std::atomic<size_t> running_readers;
    std::atomic<GliderPointerNode*> cleanup_head_ptr;  // also used as a lock
    std::atomic<GliderPointerNode*> last_cleaned_head_ptr;

    void cleanup();

    public:
        GliderPointer(ConcurrentGarbageCollector* cgc, ConcurrentGCItem* item_ptr);
        virtual ~GliderPointer();

        GliderPointerNode* head();
        bool replace_head(ConcurrentGCItem* next_head, GliderPointerNode* prev_head=NULL);

        template<typename T>
        GliderContext<T> head_context(){
            return GliderContext<T>(this->head());
        }
};

};  // namespace wasp::cgc

#endif  // __WASP_C_EXTENSIONS__CGC_GLIDER_PTR_HPP__
