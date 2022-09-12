// wasp_c_extensions/_cgc/cgc.cpp
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

#include "cgc.hpp"

using namespace wasp::cgc;

PointerDestructor::PointerDestructor(void (*fn)(PointerDestructor*)):
    destroy_fn(fn)
{
    if (! this->destroy_fn) {
        throw NullPointerException();
    }
}

PointerDestructor::~PointerDestructor(){}

void PointerDestructor::destroy(PointerDestructor* item_ptr){

    void (*destroy_fn)(wasp::cgc::PointerDestructor*) = NULL;

    if (! item_ptr) {
        throw NullPointerException();
    }

    destroy_fn = item_ptr->destroy_fn;
    destroy_fn(item_ptr);
}

ConcurrentGCItem::ConcurrentGCItem(void (*fn)(PointerDestructor*)):
    PointerDestructor(fn),
    gc_ready_flag(false),
    gc_next(NULL)
{}

void ConcurrentGCItem::destroyable()
{
    this->gc_ready_flag = true;
}

ConcurrentGCItem::~ConcurrentGCItem()
{
    assert(this->gc_ready_flag == true);
}

ConcurrentGarbageCollector::ConcurrentGarbageCollector():
    head(NULL),
    parallel_gc(0),
    count(0)
{}

ConcurrentGarbageCollector::~ConcurrentGarbageCollector()
{
    assert(this->parallel_gc.load(std::memory_order_seq_cst) == 0);
    this->collect();
    assert(this->count.load(std::memory_order_seq_cst) == 0);
}

size_t ConcurrentGarbageCollector::items(){
    return this->count.load(std::memory_order_seq_cst);
}

void ConcurrentGarbageCollector::push(ConcurrentGCItem* item_ptr){
    if (! item_ptr) {
        throw NullPointerException();
    }

    if (item_ptr->gc_ready_flag == true) {
        throw InvalidItemState();
    }

    while (! this->__push(item_ptr, item_ptr));
    this->count.fetch_add(1, std::memory_order_seq_cst);
}

bool ConcurrentGarbageCollector::__push(ConcurrentGCItem* new_head_ptr, ConcurrentGCItem* new_tail_ptr)
{
    ConcurrentGCItem* current_head = this->head.load(std::memory_order_seq_cst);
    new_tail_ptr->gc_next.store(current_head, std::memory_order_seq_cst);
    return this->head.compare_exchange_strong(current_head, new_head_ptr, std::memory_order_seq_cst);
}

ConcurrentGCItem* ConcurrentGarbageCollector::detach_head()
{
    ConcurrentGCItem* current_head = NULL;

    do {
        current_head = this->head.load(std::memory_order_seq_cst);
    }
    while (! this->head.compare_exchange_strong(current_head, NULL, std::memory_order_seq_cst));
    return current_head;
}

void ConcurrentGarbageCollector::collect()
{
    ConcurrentGCItem *head_ptr = NULL;  // pointer to a new head
    ConcurrentGCItem *tail_ptr = NULL;  // pointer to a new tail
    ConcurrentGCItem *next_ptr = NULL;  // pointer to a new item to check next
    ConcurrentGCItem *current_ptr = this->detach_head();  // current item to check

    this->parallel_gc.fetch_add(1, std::memory_order_seq_cst);

    while(current_ptr){
        next_ptr = current_ptr->gc_next.load(std::memory_order_seq_cst);

        if (current_ptr->gc_ready_flag){
            ConcurrentGCItem::destroy(current_ptr);
            this->count.fetch_sub(1, std::memory_order_seq_cst);

            if (tail_ptr && tail_ptr->gc_next.load(std::memory_order_seq_cst) == current_ptr){
                tail_ptr->gc_next.store(NULL, std::memory_order_seq_cst);
            }
            current_ptr = next_ptr;
            continue;
        }

        if (! head_ptr){
            head_ptr = current_ptr;
        }

        if (tail_ptr){
            tail_ptr->gc_next.store(current_ptr, std::memory_order_seq_cst);
        }
        tail_ptr = current_ptr;

        current_ptr = next_ptr;
    }

    if (head_ptr && tail_ptr){
        while (! this->__push(head_ptr, tail_ptr));
    }

    this->parallel_gc.fetch_sub(1, std::memory_order_seq_cst);
}

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

SmartPointer::SmartPointer(PointerDestructor* p):
    pointer_lock(),
    pointer(p),
    zombie_pointer(NULL)
{
    if (! p) {
        throw NullPointerException();
    }

    assert(this->pointer_lock.reset());
}

SmartPointer::~SmartPointer(){}

PointerDestructor* SmartPointer::acquire(){
    PointerDestructor* pointer = this->pointer.load(std::memory_order_seq_cst);

    if (this->pointer_lock.acquire()){
        return pointer;
    }
    return NULL;
}

void SmartPointer::release(){

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

bool SmartPointer::replace(PointerDestructor* new_ptr){

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

CGCSmartPointer::CGCSmartPointer(void (*destroy_fn)(PointerDestructor*), PointerDestructor* p):
    ConcurrentGCItem(destroy_fn),
    SmartPointer(p),
    destroyable_request_flag(false),
    destroyable_commit_flag(false),
    pending_releases(1)
{}

CGCSmartPointer::~CGCSmartPointer()
{
    assert(this->destroyable_request_flag);
    assert(this->destroyable_commit_flag.load(std::memory_order_seq_cst));
    assert(this->pending_releases.load(std::memory_order_seq_cst) == 0);
}

void CGCSmartPointer::destroyable(){
    this->destroyable_request_flag = true;
    this->check_and_fall();
}

void CGCSmartPointer::check_and_fall()
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

PointerDestructor* CGCSmartPointer::acquire(){
    PointerDestructor* result = NULL;

    this->pending_releases.fetch_add(1, std::memory_order_seq_cst);

    if (! this->destroyable_request_flag){
        result = this->SmartPointer::acquire();
        if (result){
            return result;
        }
    }

    this->pending_releases.fetch_sub(1, std::memory_order_seq_cst);
    this->check_and_fall();
    return NULL;
}

void CGCSmartPointer::release(){
    this->SmartPointer::release();
    this->pending_releases.fetch_sub(1, std::memory_order_seq_cst);
    this->check_and_fall();
}

bool CGCSmartPointer::replace(PointerDestructor* new_ptr){
    if (! this->destroyable_request_flag){
        if (this->SmartPointer::replace(new_ptr)){
            this->pending_releases.fetch_add(1, std::memory_order_seq_cst);
            return true;
        };
        return false;
    }

    this->check_and_fall();
    return false;
}
