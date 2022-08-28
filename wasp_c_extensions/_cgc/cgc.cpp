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

ConcurrentGCItem::ConcurrentGCItem(void (*fn)(ConcurrentGCItem*)):
    destroy_fn(fn),
    gc_ready(false),
    next(NULL)
{
    if (! this->destroy_fn) {
        throw NullPointerException();
    }
}

void ConcurrentGCItem::destroy(ConcurrentGCItem* item_ptr){

    void (*destroy_fn)(wasp::cgc::ConcurrentGCItem*) = NULL;

    if (! item_ptr) {
        throw NullPointerException();
    }

    destroy_fn = item_ptr->destroy_fn;
    destroy_fn(item_ptr);
}

ConcurrentGCItem::~ConcurrentGCItem()
{
    assert(this->gc_ready.load(std::memory_order_seq_cst) == true);
}

ConcurrentGarbageCollector::ConcurrentGarbageCollector():
    head(NULL)
{}

ConcurrentGarbageCollector::~ConcurrentGarbageCollector()
{
    this->collect();
}

void ConcurrentGarbageCollector::push(ConcurrentGCItem* item_ptr){
    if (! item_ptr) {
        throw NullPointerException();
    }

    if (item_ptr->gc_ready.load(std::memory_order_seq_cst) == true) {
        throw InvalidItemState();
    }

    while (! this->__push(item_ptr, item_ptr));
}

bool ConcurrentGarbageCollector::__push(ConcurrentGCItem* new_head_ptr, ConcurrentGCItem* new_tail_ptr)
{
    ConcurrentGCItem* current_head = this->head.load(std::memory_order_seq_cst);

    new_tail_ptr->next.store(current_head, std::memory_order_seq_cst);
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

    while(current_ptr){
        next_ptr = current_ptr->next.load(std::memory_order_seq_cst);

        if (current_ptr->gc_ready.load(std::memory_order_seq_cst)){
            ConcurrentGCItem::destroy(current_ptr);
            current_ptr = next_ptr;
            continue;
        }

        if (! head_ptr){
            head_ptr = current_ptr;
        }

        if (tail_ptr){
            tail_ptr->next.store(next_ptr, std::memory_order_seq_cst);
        }
        tail_ptr = current_ptr;

        current_ptr = next_ptr;
    }

    if (head_ptr && tail_ptr){
        while (! this->__push(head_ptr, tail_ptr));
    }
}

ConcurrentGCItem* ConcurrentGarbageCollector::pop()
{
    // TODO: implement
    return NULL;
}
