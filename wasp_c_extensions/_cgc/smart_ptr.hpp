// wasp_c_extensions/_cgc/smart_ptr.hpp
//
//Copyright (C) 2022-2023 the wasp-c-extensions authors and contributors
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

#ifndef __WASP_C_EXTENSIONS__CGC_SMART_PTR_HPP__
#define __WASP_C_EXTENSIONS__CGC_SMART_PTR_HPP__

#include <atomic>
#include <cassert>
#include <cstddef>

#include "cgc.hpp"

namespace wasp::cgc {

class ShockSemaphore
{

    std::atomic<bool>   dead_flag;                  // marks this resource as unavailable
    std::atomic<size_t> usage_counter;              // this counter shows how many pending "releases" there are.
    // This counter is for managing acquire-release concurrency
    std::atomic<bool>   concurrency_liveness_flag;  // This flag indicate the "acquire" method that the "release" is
    // on the fly

    public:
        ShockSemaphore();
        virtual ~ShockSemaphore();

        virtual bool acquire();  // acknowledge that a shared resource is requested
        // (returns true if the resource is available)

        virtual bool release();  // acknowledge that a shared resource is no longer in use. For every successful
        // "acquire" a single release must be called. (returns true if the resource is ready to be destroyed)

        size_t counter();

        bool is_dead();
};

class SmartPointerBase
{
    ShockSemaphore pointer_semaphore;
    std::atomic<PointerDestructor*> pointer;

    public:
        SmartPointerBase(); // TODO: to the protected section?
        virtual ~SmartPointerBase();

        size_t usage_counter();

        virtual void release();
        virtual bool is_dead();

    protected:
        virtual PointerDestructor* acquire();
        virtual bool init(PointerDestructor* new_ptr);
};

template <typename T1, typename T2>
T1 ensure_cast(T2 t2_ptr){
    T1 t1_ptr = NULL;

    if (t2_ptr){
        t1_ptr = dynamic_cast<T1>(t2_ptr);
        assert(t1_ptr);
    }

    return t1_ptr;
}

template <typename T>
class SmartPointer:
    public SmartPointerBase
{
    static_assert(std::is_base_of<PointerDestructor, T>::value, "T must extend PointerDestructor");

    public:

        T* acquire(){
            return ensure_cast<T*>(SmartPointerBase::acquire());
        }

        bool init(T* new_ptr){
            return SmartPointerBase::init(new_ptr);
        }
};

template <typename T>
class CGCSmartPointer:
    public ConcurrentGCItem
{
    SmartPointer<T>     smart_ptr;
    std::atomic<size_t> pending_releases;  // just a counter, but more accurate then SmartPointer since SmartPointer
    // may be overlapped/overflowed

    virtual void destroyable(){  // just to hide the ConcurrentGCItem method
        this->ConcurrentGCItem::destroyable();
    }

    public:

        CGCSmartPointer(void (*destroy_fn)(PointerDestructor*)):
            ConcurrentGCItem(destroy_fn)
        {}

        virtual ~CGCSmartPointer(){
            assert(this->smart_ptr.is_dead());
        }

        T* acquire(){
            T* result = NULL;
            this->pending_releases.fetch_add(1, std::memory_order_seq_cst);

            result = this->smart_ptr.acquire();
            if (result){
                return result;
            }

            this->pending_releases.fetch_sub(1, std::memory_order_seq_cst);
            return NULL;
        }

        void release(){
            this->smart_ptr.release();
            this->pending_releases.fetch_sub(1, std::memory_order_seq_cst);

            if (this->smart_ptr.is_dead()){
                this->destroyable();
            }
        };

        bool init(T* new_ptr){
            if (this->smart_ptr.init(new_ptr)){
                this->pending_releases.fetch_add(1, std::memory_order_seq_cst);
                return true;
            }
            return false;
        }

        void gc_item_id(std::ostream& os){
            void* ptr = this->acquire();
            size_t counter = this->smart_ptr.usage_counter();
            size_t releases = this->pending_releases.load(std::memory_order_seq_cst);

            if (ptr){
                counter -= 1;
                releases -= 1;
            }

            os << "CGCSmartPointer object [pointer: " << ptr;
            os << ", counter: " << counter << ", releases: " << releases << "]";

            if (ptr){
                this->release();
            }
        }
};

};  // namespace wasp::cgc

#endif  //  __WASP_C_EXTENSIONS__CGC_SMART_PTR_HPP__
