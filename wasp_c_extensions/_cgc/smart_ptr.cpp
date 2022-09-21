// wasp_c_extensions/_cgc/smart_ptr.cpp
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

#include "smart_ptr.hpp"

using namespace wasp::cgc;

ResourceSmartLock::ResourceSmartLock():
    is_dead(true),
    usage_counter(0),
    concurrency_call_counter(0),
    concurrency_liveness_flag(false)
{}

ResourceSmartLock::~ResourceSmartLock(){
    assert(this->is_dead.load(std::memory_order_seq_cst) == true);
    assert(this->usage_counter.load(std::memory_order_seq_cst) == 0);
};

bool ResourceSmartLock::able_to_reset(){
    return (this->is_dead.load(std::memory_order_seq_cst) == true) &&
        (this->usage_counter.load(std::memory_order_seq_cst) == 0) &&
        (this->concurrency_call_counter.load(std::memory_order_seq_cst) == 0);
}

bool ResourceSmartLock::reset()
{
    if (
        (this->is_dead.load(std::memory_order_seq_cst) == true) &&
        (this->usage_counter.load(std::memory_order_seq_cst) == 0) &&
        (this->concurrency_call_counter.load(std::memory_order_seq_cst) == 0)
    ){
        this->usage_counter.store(1, std::memory_order_seq_cst);
        this->is_dead.store(false, std::memory_order_seq_cst);

        return true;
    }

    return false;
}

bool ResourceSmartLock::acquire(){
    if (this->is_dead.load(std::memory_order_seq_cst)){
        return false;
    }

    this->concurrency_call_counter.fetch_add(1, std::memory_order_seq_cst);

    this->concurrency_liveness_flag.store(true, std::memory_order_seq_cst);

    this->usage_counter.fetch_add(1, std::memory_order_seq_cst);

    if(! this->concurrency_liveness_flag.load(std::memory_order_seq_cst)){
        this->concurrency_call_counter.fetch_sub(1, std::memory_order_seq_cst);
        return false;
    }

    this->concurrency_call_counter.fetch_sub(1, std::memory_order_seq_cst);

    return true;
}

bool ResourceSmartLock::release(){
    size_t sub_result = 0, sub_overflow_value = -1;

    sub_result = this->usage_counter.fetch_sub(1, std::memory_order_seq_cst);
    assert(sub_result != sub_overflow_value);

    if (sub_result > 1){
        return false;
    }

    this->concurrency_liveness_flag.store(false, std::memory_order_seq_cst);

    if (this->usage_counter.load(std::memory_order_seq_cst)){
        return false;  // parallel acquire on the way
    }

    this->is_dead.store(true, std::memory_order_seq_cst);

    return true;
}

SmartPointerBase::SmartPointerBase():
    pointer_lock(),
    pointer(NULL),
    zombie_pointer(NULL)
{}

SmartPointerBase::~SmartPointerBase(){}

PointerDestructor* SmartPointerBase::acquire(){
    PointerDestructor* pointer = this->pointer.load(std::memory_order_seq_cst);

    if (this->pointer_lock.acquire()){
        return pointer;
    }
    return NULL;
}

void SmartPointerBase::release(){

    PointerDestructor *pointer = this->pointer.load(std::memory_order_seq_cst);

    assert(pointer != NULL);

    if (this->pointer_lock.release()){
        if (this->pointer.compare_exchange_strong(pointer, NULL, std::memory_order_seq_cst)){
            pointer->destroyable();
            return;
        }
        assert(0);  // there must not be a concurrency at this point
    }
}

bool SmartPointerBase::replace(PointerDestructor* new_ptr){

    PointerDestructor* null_ptr = this->pointer.load(std::memory_order_seq_cst);

    assert(new_ptr != NULL);

    if (null_ptr){
        return false;
    }

    if (this->pointer_lock.able_to_reset()){
        if (this->zombie_pointer.compare_exchange_strong(null_ptr, new_ptr, std::memory_order_seq_cst)) {
            if (this->pointer_lock.able_to_reset()){
                if (this->pointer_lock.reset()){
                    if (this->pointer.compare_exchange_strong(null_ptr, new_ptr, std::memory_order_seq_cst)){
                        this->zombie_pointer.store(NULL, std::memory_order_seq_cst);
                        return true;
                    }
                    assert(0);
                }
                assert(0);
            }
            this->zombie_pointer.store(NULL, std::memory_order_seq_cst);
        }
    }

    return false;
}

bool SmartPointerBase::swap(PointerDestructor* new_ptr){
    if (this->replace(new_ptr)){
        return true;
    }

    //
    return false;
}

CGCSmartPointerBase::CGCSmartPointerBase(void (*destroy_fn)(PointerDestructor*)):
    ConcurrentGCItem(destroy_fn),
    block_mode_flag(false),
    destroyable_request_flag(false),
    destroyable_commit_flag(false),
    pending_releases(0)
{}

CGCSmartPointerBase::~CGCSmartPointerBase()
{
    assert(this->block_mode_flag);
    assert(this->destroyable_request_flag);
    assert(this->destroyable_commit_flag.load(std::memory_order_seq_cst));
    assert(this->pending_releases.load(std::memory_order_seq_cst) == 0);
}

void CGCSmartPointerBase::block_mode(){
    this->block_mode_flag = true;
}

void CGCSmartPointerBase::destroyable(){
    assert(this->block_mode_flag);
    this->destroyable_request_flag = true;
    this->check_and_fall();
}

void CGCSmartPointerBase::check_and_fall()
{
    if (! this->destroyable_commit_flag.load(std::memory_order_seq_cst)){
        if (this->destroyable_request_flag){

            if (this->pending_releases.load(std::memory_order_seq_cst) == 0){
                if (! this->destroyable_commit_flag.exchange(true, std::memory_order_seq_cst)){
                    this->ConcurrentGCItem::destroyable();
                }
            }
        }
    }
}

PointerDestructor* CGCSmartPointerBase::wrapped_acquire(){
    PointerDestructor* result = NULL;

    this->pending_releases.fetch_add(1, std::memory_order_seq_cst);

    if (! this->block_mode_flag){
        result = this->smart_acquire();
        if (result){
            return result;
        }
    }

    this->pending_releases.fetch_sub(1, std::memory_order_seq_cst);
    this->check_and_fall();
    return NULL;
}

void CGCSmartPointerBase::wrapped_release(){
    this->smart_release();
    this->pending_releases.fetch_sub(1, std::memory_order_seq_cst);
    this->check_and_fall();
}

bool CGCSmartPointerBase::wrapped_replace(PointerDestructor* new_ptr){

    this->pending_releases.fetch_add(1, std::memory_order_seq_cst);
    if (! this->block_mode_flag){
        if (this->smart_replace(new_ptr)){
            return true;
        };
    }
    this->pending_releases.fetch_sub(1, std::memory_order_seq_cst);
    this->check_and_fall();
    return false;
}
