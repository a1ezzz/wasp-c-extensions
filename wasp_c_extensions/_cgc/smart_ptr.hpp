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

    std::atomic<bool>   dead_flag;                  // marks this resource as unavailable
    std::atomic<size_t> usage_counter;              // this counter shows how many pending "releases" there are.
    // This counter is for managing acquire-release concurrency
    std::atomic<bool>   concurrency_liveness_flag;  // This flag indicate the "acquire" method that the "release" is
    // on the fly

    protected:

        void resurrect();  // is not thread-safe

    public:
        ResourceSmartLock();
        virtual ~ResourceSmartLock();

        size_t counter();

        virtual bool acquire();  // acknowledge that a shared resource is requested (returns true if the resource is available)

        virtual bool release();  // acknowledge that a shared resource is no longer in use. For every successful "acquire"
        // a single release must be called. (returns true if the resource is ready to be destroyed)

        bool is_dead();
};

class RenewableSmartLock:
    public ResourceSmartLock
{
    // TODO: renew and test!

    std::atomic_flag is_renewing;
    std::atomic<bool> is_available;

    public:
        RenewableSmartLock():
            ResourceSmartLock(),
            is_renewing(false),
            is_available(true)
        {}

        bool acquire(){
            if (this->is_available.load(std::memory_order_seq_cst)){
                return ResourceSmartLock::acquire();
            }

            return false;
        }

        bool release(){
            bool true_v = true, result = ResourceSmartLock::release();

            if (result){
                this->is_available.compare_exchange_strong(true_v, false, std::memory_order_seq_cst);
            }
            return result;
        }

        bool renew(){
            bool false_v = false;

            if (! this->is_available.load(std::memory_order_seq_cst)){
                if (! this->is_renewing.test_and_set(std::memory_order_seq_cst) && this->is_dead()){
                    // this removes concurrency
                    this->resurrect();
                    this->is_renewing.clear();
                    assert(this->is_available.compare_exchange_strong(false_v, true, std::memory_order_seq_cst));
                    return true;
                }
            }

            return false;
        };
};

class SmartPointerBase
{
    ResourceSmartLock pointer_lock;
    std::atomic<PointerDestructor*> pointer;

    public:
        SmartPointerBase();
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

class CGCSmartPointerBase:
    public ConcurrentGCItem
{
    volatile bool destroyable_request_flag;
    std::atomic<size_t> pending_releases;  // just a counter

    void check_and_fall();

    protected:

        virtual PointerDestructor* smart_acquire() = 0;
        virtual void smart_release() = 0;
        virtual bool smart_init(PointerDestructor* new_ptr) = 0;
        virtual bool smart_is_dead() = 0;

        // wrapped SmartPointer methods
        PointerDestructor* wrapped_acquire();
        void wrapped_release();
        bool wrapped_init(PointerDestructor* new_ptr);

    public:
        CGCSmartPointerBase(void (*destroy_fn)(PointerDestructor*));
        virtual ~CGCSmartPointerBase();

        size_t releases();

        // ConcurrentGCItem method
        virtual void destroyable();  // concurrency with release
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

        bool smart_init(PointerDestructor* new_ptr){
            return SmartPointer<T>::init(dynamic_cast<T*>(new_ptr));
        }

        bool smart_is_dead(){
            return SmartPointer<T>::is_dead();
        }

    public:

        CGCSmartPointer(void (*destroy_fn)(PointerDestructor*)):
            SmartPointer<T>(),
            CGCSmartPointerBase(destroy_fn)
        {}

        virtual ~CGCSmartPointer(){
            assert(this->smart_is_dead());
        }

        T* acquire(){
            return ensure_cast<T*>(this->wrapped_acquire());
        }

        void release(){
            this->wrapped_release();
        };

        bool init(T* new_ptr){
            return this->wrapped_init(new_ptr);
        }

        void gc_item_id(std::ostream& os){
            void* ptr = this->wrapped_acquire();
            size_t counter = this->usage_counter();
            size_t releases = this->releases();

            if (ptr){
                counter -= 1;
                releases -= 1;
            }

            os << "CGCSmartPointer object [pointer: " << ptr;
            os << ", counter: " << counter << ", releases: " << releases << "]";

            if (ptr){
                this->wrapped_release();
            }
        }
};

template <typename T>
class SmartDualPointer {

    std::atomic<CGCSmartPointer<T>*> atomic_current_ptr;
    std::atomic<CGCSmartPointer<T>*> atomic_next_ptr;

    RenewableSmartLock current_ptr_lock;
    RenewableSmartLock next_ptr_lock;

    std::atomic_flag is_switching;

    protected:
        virtual void release_prev(CGCSmartPointer<T>* prev_ptr, CGCSmartPointer<T>* curr_ptr){
            assert(prev_ptr);
            prev_ptr->release();
        }

    public:
        SmartDualPointer():
            atomic_current_ptr(NULL),
            atomic_next_ptr(NULL),
            current_ptr_lock(),
            next_ptr_lock(),
            is_switching(false)
        {
            this->current_ptr_lock.release();  // since the lock is acquired by default
            this->next_ptr_lock.release();  // since the lock is acquired by default
        }

        virtual ~SmartDualPointer(){
            // TODO: check!
            CGCSmartPointer<T> *smart_current_ptr = NULL;

            assert(! this->atomic_next_ptr.load(std::memory_order_seq_cst));

            if (! current_ptr_lock.is_dead()){
                assert(current_ptr_lock.release());
                smart_current_ptr = this->atomic_current_ptr.load(std::memory_order_seq_cst);
                assert(smart_current_ptr);
                smart_current_ptr->release();
            }
        }

        CGCSmartPointer<T>* acquire_next(){
            CGCSmartPointer<T> *result = NULL;

            if (this->next_ptr_lock.acquire()){
                result = this->atomic_next_ptr.load(std::memory_order_seq_cst);
                assert(result);
                assert(result->acquire());
                this->next_ptr_lock.release();
                this->switch_over();
            }
            else if (this->current_ptr_lock.acquire()){
                result = this->atomic_current_ptr.load(std::memory_order_seq_cst);
                assert(result);
                assert(result->acquire());
                this->current_ptr_lock.release();
                this->switch_over();
            }
            return result;
        }

        bool setup_next(CGCSmartPointer<T>* next_ptr){
            bool result = false;
            CGCSmartPointer<T> *null_ptr = NULL, *acquired_item = NULL;

            assert(next_ptr);
            assert(next_ptr->acquire());

            if (this->atomic_current_ptr.compare_exchange_strong(null_ptr, next_ptr, std::memory_order_seq_cst)){
                // this is the first call of the setup_next method
                assert(this->current_ptr_lock.renew());
                return true;
            }

            null_ptr = NULL;  // must be set because of the previous compare_exchange_strong call

            if (this->next_ptr_lock.is_dead()){

            result = this->atomic_next_ptr.compare_exchange_strong(null_ptr, next_ptr, std::memory_order_seq_cst);
            if(result){
                assert(this->next_ptr_lock.renew());
                this->current_ptr_lock.release();
                this->switch_over();
            }
                return true;
            }

            next_ptr->release();
            return false;
        }

        void switch_over(){
            CGCSmartPointer<T> *curr_item = NULL, *next_item = NULL;

            if (! this->is_switching.test_and_set(std::memory_order_seq_cst)){

                if (this->current_ptr_lock.is_dead()){

                    if (this->next_ptr_lock.acquire()){
                        next_item = this->atomic_next_ptr.load(std::memory_order_seq_cst);
                        curr_item = this->atomic_current_ptr.load(std::memory_order_seq_cst);

                        assert(
                            this->atomic_current_ptr.compare_exchange_strong(curr_item, next_item, std::memory_order_seq_cst)
                        );

                        this->current_ptr_lock.renew();
                        this->next_ptr_lock.release();
                        this->next_ptr_lock.release();

                        this->release_prev(curr_item, next_item);
                    }
                }

                if (this->next_ptr_lock.is_dead()){
                        this->atomic_next_ptr.store(NULL, std::memory_order_seq_cst);
                }

                this->is_switching.clear();
            }
        }

        bool next_replaceable(){
            // TODO: looks like crap; remove this method
            return this->next_ptr_lock.is_dead();
        }
};

};  // namespace wasp::cgc

#endif  //  __WASP_C_EXTENSIONS__CGC_SMART_PTR_HPP__
