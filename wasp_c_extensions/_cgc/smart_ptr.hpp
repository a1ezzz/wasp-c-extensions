// wasp_c_extensions/_cgc/smart_ptr.hpp
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

#ifndef __WASP_C_EXTENSIONS__CGC_SMART_PTR_HPP__
#define __WASP_C_EXTENSIONS__CGC_SMART_PTR_HPP__

#include <atomic>
#include <cassert>
#include <cstddef>

#include "cgc.hpp"

namespace wasp::cgc {

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

        size_t counter();

        bool able_to_reset();  // check that a resource is capable to be replaced

        bool reset();  // return true, if internal counter/flags are reset. This indicated that resource may be changed
        // multiple concurrent requests to this method will make errors

        bool acquire();  // acknowledge that a shared resource is requested (returns true if the resource is available)

        bool release();  // acknowledge that a shared resource is no longer in use. For every successful "acquire"
        // a single release must be called. (returns true if the resource is ready to be destroyed)
};

class SmartPointerBase
{
    ResourceSmartLock pointer_lock;
    std::atomic<PointerDestructor*> pointer;
    std::atomic<PointerDestructor*> zombie_pointer;  // this pointer protects the pointer_lock.reset method
    // from being called concurrent

    public:
        SmartPointerBase();
        virtual ~SmartPointerBase();

        size_t usage_counter();

    protected:
        virtual PointerDestructor* acquire();
        virtual void release();
        virtual bool replace(PointerDestructor* new_ptr);
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

        void release(){
            SmartPointerBase::release();
        }

        bool replace(T* new_ptr){
            return SmartPointerBase::replace(new_ptr);
        }
};

class CGCSmartPointerBase:
    public ConcurrentGCItem
{
    // There is still some concurrency issue. Like when acquire or replace methods are called after the destroyable
    // method then SegFault may happen because object may be destroyed already
    // So it is better to be sure that there will be no request to acquire/replace methods in future when the
    // destroyable method is called.
    // In order to block acquire/replace requests -- there is a "block_mode" method

    // TODO: get rid off replace?

    volatile bool block_mode_flag;
    volatile bool destroyable_request_flag;
    std::atomic<bool> destroyable_commit_flag;
    std::atomic<size_t> pending_releases;

    void check_and_fall();

    protected:

        virtual PointerDestructor* smart_acquire() = 0;
        virtual void smart_release() = 0;
        virtual bool smart_replace(PointerDestructor* new_ptr) = 0;

        // wrapped SmartPointer methods
        PointerDestructor* wrapped_acquire();
        void wrapped_release();
        bool wrapped_replace(PointerDestructor* new_ptr);

    public:
        CGCSmartPointerBase(void (*destroy_fn)(PointerDestructor*));
        virtual ~CGCSmartPointerBase();

        void block_mode();
        size_t releases();

        // ConcurrentGCItem method
        virtual void destroyable();  // concurrency with release/replace
};


template <typename T>
class CGCSmartPointer:
    public SmartPointer<T>,
    public CGCSmartPointerBase
{
    protected:

        PointerDestructor* smart_acquire(){
            return SmartPointer<T>::acquire();
        }

        void smart_release(){
            SmartPointer<T>::release();
        }

        bool smart_replace(PointerDestructor* new_ptr){
            return SmartPointer<T>::replace(dynamic_cast<T*>(new_ptr));
        }

    public:

        CGCSmartPointer(void (*destroy_fn)(PointerDestructor*)):
            SmartPointer<T>(),
            CGCSmartPointerBase(destroy_fn)
        {}

        T* acquire(){
            return ensure_cast<T*>(this->wrapped_acquire());
        }

        void release(){
            this->wrapped_release();
        };

        bool replace(T* new_ptr){
            return this->wrapped_replace(new_ptr);
        }

        void gc_item_id(std::ostream& os){
            void* ptr = this->wrapped_acquire();
            size_t counter = this->usage_counter();
            size_t releases = this->releases();

            if (ptr){
                counter -= 1;
                releases -= 1;
            }

            os << "CGCSmartPointer object [pointer: " << ptr << ", counter: " << counter << ", releases: " << releases << "]";

            if (ptr){
                this->wrapped_release();
            }
        }
};

};  // namespace wasp::cgc

#endif  //  __WASP_C_EXTENSIONS__CGC_SMART_PTR_HPP__
