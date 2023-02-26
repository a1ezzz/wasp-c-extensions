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
    dead_flag(false),
    usage_counter(1),
    concurrency_liveness_flag(false)
{}

ResourceSmartLock::~ResourceSmartLock(){
    assert(this->dead_flag.load(std::memory_order_seq_cst) == true);
    assert(this->usage_counter.load(std::memory_order_seq_cst) == 0);
};

size_t ResourceSmartLock::counter(){
    return this->usage_counter.load(std::memory_order_seq_cst);
}

void ResourceSmartLock::resurrect(){
    bool true_v = true;
    assert(this->dead_flag.load(std::memory_order_seq_cst));
    this->usage_counter.store(1, std::memory_order_seq_cst);
    this->concurrency_liveness_flag.store(false, std::memory_order_seq_cst);
    assert(this->dead_flag.compare_exchange_strong(true_v, false, std::memory_order_seq_cst));
}

bool ResourceSmartLock::acquire(){
    if (this->dead_flag.load(std::memory_order_seq_cst)){
        return false;
    }

    this->concurrency_liveness_flag.store(true, std::memory_order_seq_cst);

    this->usage_counter.fetch_add(1, std::memory_order_seq_cst);

    if(! this->concurrency_liveness_flag.load(std::memory_order_seq_cst)){
        this->usage_counter.fetch_sub(1, std::memory_order_seq_cst);
        return false;
    }

    return true;
}

bool ResourceSmartLock::release(){
    if (this->usage_counter.fetch_sub(1, std::memory_order_seq_cst) > 1){
        return false;
    }

    this->concurrency_liveness_flag.store(false, std::memory_order_seq_cst);

    if (this->usage_counter.load(std::memory_order_seq_cst)){
        return false;  // parallel acquire on the way
    }

    this->dead_flag.store(true, std::memory_order_seq_cst);
    return true;
}

bool ResourceSmartLock::is_dead(){
    return this->dead_flag.load(std::memory_order_seq_cst);
}

RenewableSmartLock::RenewableSmartLock():
    ResourceSmartLock(),
    is_renewing(false),
    renew_lock()
{}

bool RenewableSmartLock::acquire(){
    if (this->renew_lock.acquire()){
        if (this->ResourceSmartLock::acquire()){
            this->renew_lock.release();
            return true;
        }
        this->renew_lock.release();
    }
    return false;
}

bool RenewableSmartLock::release(){
    bool result = this->ResourceSmartLock::release();

    if (result){
        this->renew_lock.release();
    }
    return result;
}

bool RenewableSmartLock::renew(){

    if (this->is_renewing.test_and_set(std::memory_order_seq_cst)){
        return false;
    }

    if (! this->renew_lock.is_dead()){
        this->is_renewing.clear();
        return false;
    }

    this->resurrect();
    this->renew_lock.resurrect();

    this->is_renewing.clear();
    return true;
};

SmartPointerBase::SmartPointerBase():
    pointer_lock(),
    pointer(NULL)
{}

SmartPointerBase::~SmartPointerBase(){}

PointerDestructor* SmartPointerBase::acquire(){
    PointerDestructor* pointer = this->pointer.load(std::memory_order_seq_cst);

    if (pointer && this->pointer_lock.acquire()){
        return pointer;
    }
    return NULL;
}

size_t SmartPointerBase::usage_counter(){
    return this->pointer_lock.counter();
}

void SmartPointerBase::release(){
    PointerDestructor* pointer = this->pointer.load(std::memory_order_seq_cst);
    assert(pointer);
    if (this->pointer_lock.release()){
        pointer->destroyable();
    }
}

bool SmartPointerBase::init(PointerDestructor* new_ptr){
    PointerDestructor* null_ptr = NULL;
    assert(new_ptr != NULL);
    return this->pointer.compare_exchange_strong(null_ptr, new_ptr, std::memory_order_seq_cst);
}

bool SmartPointerBase::is_dead(){
    return this->pointer_lock.is_dead();
}

CGCSmartPointerBase::CGCSmartPointerBase(void (*destroy_fn)(PointerDestructor*)):
    ConcurrentGCItem(destroy_fn),
    destroyable_request_flag(false),
    pending_releases(0)
{}

CGCSmartPointerBase::~CGCSmartPointerBase()
{
    assert(this->destroyable_request_flag);
    assert(this->pending_releases.load(std::memory_order_seq_cst) == 0);
}

size_t CGCSmartPointerBase::releases(){
    return this->pending_releases.load(std::memory_order_seq_cst);
}

void CGCSmartPointerBase::destroyable(){
    this->destroyable_request_flag = true;
    this->check_and_fall();
}

void CGCSmartPointerBase::check_and_fall()
{
    if (this->destroyable_request_flag && this->smart_is_dead()){
        this->ConcurrentGCItem::destroyable();
    }
}

PointerDestructor* CGCSmartPointerBase::wrapped_acquire(){
    PointerDestructor* result = NULL;

    this->pending_releases.fetch_add(1, std::memory_order_seq_cst);

    result = this->smart_acquire();
    if (result){
        return result;
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

bool CGCSmartPointerBase::wrapped_init(PointerDestructor* new_ptr){
    this->pending_releases.fetch_add(1, std::memory_order_seq_cst);
    return this->smart_init(new_ptr);
}
