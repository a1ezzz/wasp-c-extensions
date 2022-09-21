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

const char* ConcurrentGCItem::gc_item_id(){
    return NULL;
}

void ConcurrentGCItem::heap_destroy_fn(PointerDestructor* ptr){
    delete ptr;
}

void ConcurrentGCItem::stack_destroy_fn(PointerDestructor* ptr){
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

void ConcurrentGarbageCollector::dump_items_to_clog(){
    ConcurrentGCItem *next_ptr = NULL;  // pointer to a new item to check next
    ConcurrentGCItem *head_ptr = this->detach_head();  // current item to check
    ConcurrentGCItem *tail_ptr = NULL;  // pointer to a new tail
    const char* gc_item_id = NULL;

    this->parallel_gc.fetch_add(1, std::memory_order_seq_cst);

    next_ptr = head_ptr;

    std::clog << "=== Dump items of the gc: ===" << std::endl;

    while(next_ptr){
        if (next_ptr){
            tail_ptr = next_ptr;
        }

        if (! next_ptr->gc_ready_flag){
            gc_item_id = next_ptr->gc_item_id();

            std::clog << "\t" << ((gc_item_id != NULL) ? gc_item_id : "<unknown item>")  << ": ";
            std::clog << next_ptr << std::endl;
        }

        next_ptr = next_ptr->gc_next.load(std::memory_order_seq_cst);
    }

    if (head_ptr && tail_ptr){
        while (! this->__push(head_ptr, tail_ptr));
    }

    this->parallel_gc.fetch_sub(1, std::memory_order_seq_cst);

    std::clog << "=== That's all folks ===" << std::endl;
}
