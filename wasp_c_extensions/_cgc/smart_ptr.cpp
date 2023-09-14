// wasp_c_extensions/_cgc/smart_ptr.cpp
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

#include "smart_ptr.hpp"

using namespace wasp::cgc;

ShockSemaphore::ShockSemaphore():
    dead_flag(false),
    usage_counter(1),
    concurrency_liveness_flag(false)
{}

ShockSemaphore::~ShockSemaphore(){
    assert(this->dead_flag.load(std::memory_order_seq_cst) == true);
    assert(this->usage_counter.load(std::memory_order_seq_cst) == 0);
};

size_t ShockSemaphore::counter(){
    return this->usage_counter.load(std::memory_order_seq_cst);
}

bool ShockSemaphore::acquire(){
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

bool ShockSemaphore::release(){
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

bool ShockSemaphore::is_dead(){
    return this->dead_flag.load(std::memory_order_seq_cst);
}

SmartPointerBase::SmartPointerBase():
    pointer_semaphore(),
    pointer(NULL)
{}

SmartPointerBase::~SmartPointerBase(){}

PointerDestructor* SmartPointerBase::acquire(){
    PointerDestructor* pointer = this->pointer.load(std::memory_order_seq_cst);

    if (pointer && this->pointer_semaphore.acquire()){
        return pointer;
    }
    return NULL;
}

size_t SmartPointerBase::usage_counter(){
    return this->pointer_semaphore.counter();
}

void SmartPointerBase::release(){
    PointerDestructor* pointer = this->pointer.load(std::memory_order_seq_cst);
    assert(pointer);
    if (this->pointer_semaphore.release()){
        pointer->destroyable();
    }
}

bool SmartPointerBase::init(PointerDestructor* new_ptr){
    PointerDestructor* null_ptr = NULL;
    assert(new_ptr != NULL);
    return this->pointer.compare_exchange_strong(null_ptr, new_ptr, std::memory_order_seq_cst);
}

bool SmartPointerBase::is_dead(){
    return this->pointer_semaphore.is_dead();
}
