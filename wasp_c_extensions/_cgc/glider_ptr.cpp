// wasp_c_extensions/_cgc/glider_ptr.cpp
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

#include "glider_ptr.hpp"

using namespace wasp::cgc;

GliderPointerNode::GliderPointerNode(void (*destroy_fn)(PointerDestructor*), ConcurrentGCItem* ptr):
    ConcurrentGCItem(destroy_fn),
    current_ptr(ptr),
    next_node(NULL)
{
    assert(ptr != NULL);
    assert(! ptr->orphaned());
}

GliderPointerNode::~GliderPointerNode()
{
    assert(! this->current_ptr_semaphore.acquire());
    assert(! this->this_semaphore.acquire());
}

ConcurrentGCItem* GliderPointerNode::acquire_ptr()
{
    if (this->current_ptr_semaphore.acquire()){
        return this->current_ptr;
    }

    return NULL;
}

void GliderPointerNode::release_ptr(){

    if (this->current_ptr_semaphore.release()){
        this->current_ptr->destroyable();
    }
}

void GliderPointerNode::acquire_node(){
    assert(this->this_semaphore.acquire());  // must always pass since the GliderPointer is the only caller
}

void GliderPointerNode::release_node(){
    if (this->this_semaphore.release()){
        this->release_ptr();
        this->destroyable();
    }
}

GliderPointer::GliderPointer(ConcurrentGarbageCollector* cgc, ConcurrentGCItem* item_ptr):
    cgc(cgc),
    tail_ptr(NULL),
    head_ptr(NULL),
    running_readers(0),
    cleanup_head_ptr(NULL),
    last_cleaned_head_ptr(NULL)
{
    GliderPointerNode* node_ptr = NULL;

    assert(! item_ptr->orphaned());
    assert(this->cgc);

    node_ptr = new GliderPointerNode(wasp::cgc::ConcurrentGCItem::heap_destroy_fn, item_ptr);
    this->cgc->push(node_ptr);

    this->tail_ptr.store(node_ptr, std::memory_order_seq_cst);
    this->head_ptr.store(node_ptr, std::memory_order_seq_cst);
    this->last_cleaned_head_ptr.store(node_ptr, std::memory_order_seq_cst);
}

GliderPointer::~GliderPointer(){
    GliderPointerNode* gpn_tail_ptr = NULL;

    this->cleanup();

    gpn_tail_ptr = this->tail_ptr.load(std::memory_order_seq_cst);
    assert(gpn_tail_ptr == this->head_ptr.load(std::memory_order_seq_cst));

    if (gpn_tail_ptr){
        gpn_tail_ptr->release_node();
    }
}

GliderPointerNode* GliderPointer::head(){

    GliderPointerNode *result_ptr = NULL, *cleanup_head_ptr = NULL, *head_ptr = NULL;

    this->running_readers.fetch_add(1, std::memory_order_seq_cst);

    head_ptr = this->head_ptr.load(std::memory_order_seq_cst);
    cleanup_head_ptr = this->cleanup_head_ptr.load(std::memory_order_seq_cst);
    result_ptr = cleanup_head_ptr ? cleanup_head_ptr : head_ptr;
    result_ptr->acquire_node();

    this->running_readers.fetch_sub(1, std::memory_order_seq_cst);

    this->cleanup();
    return result_ptr;
}

bool GliderPointer::replace_head(ConcurrentGCItem* next_head, GliderPointerNode* prev_head){
    GliderPointerNode *atomic_exc_ptr = NULL, *gpn_null_ptr = NULL, *gpn_head_ptr = NULL, *gpn_next_ptr = NULL;

    assert(! next_head->orphaned());
    gpn_head_ptr = this->head_ptr.load(std::memory_order_seq_cst);
    gpn_next_ptr = new GliderPointerNode(wasp::cgc::ConcurrentGCItem::heap_destroy_fn, next_head);
    this->cgc->push(gpn_next_ptr);

    while (prev_head == NULL || (gpn_head_ptr == prev_head)){

        assert(gpn_head_ptr);

        atomic_exc_ptr = gpn_head_ptr;
        if (this->head_ptr.compare_exchange_strong(atomic_exc_ptr, gpn_next_ptr, std::memory_order_seq_cst)){
            assert(gpn_head_ptr->next_node.compare_exchange_strong(
                gpn_null_ptr, gpn_next_ptr, std::memory_order_seq_cst
            ));

            this->cleanup();
            return true;
        }

        gpn_head_ptr = this->head_ptr.load(std::memory_order_seq_cst);
    }

    gpn_next_ptr->release_node();

    this->cleanup();
    return false;
}

void GliderPointer::cleanup(){
    GliderPointerNode *null_ptr = NULL, *current_head = NULL, *gpn_tail_ptr = NULL, *gpn_next_ptr = NULL;
    current_head = this->head_ptr.load(std::memory_order_seq_cst);

    if (this->running_readers.load(std::memory_order_seq_cst)){
        return;
    }

    if (current_head == this->last_cleaned_head_ptr.load(std::memory_order_seq_cst)){
        return;
    }

    if (this->cleanup_head_ptr.compare_exchange_strong(null_ptr, current_head, std::memory_order_seq_cst)){

        if (this->running_readers.load(std::memory_order_seq_cst)){
            this->cleanup_head_ptr.store(NULL, std::memory_order_seq_cst);
            return;
        }

        gpn_tail_ptr = this->tail_ptr.load(std::memory_order_seq_cst);
        gpn_next_ptr = gpn_tail_ptr->next_node.load(std::memory_order_seq_cst);

        while (gpn_next_ptr && gpn_tail_ptr != current_head){

            assert(this->tail_ptr.compare_exchange_strong(gpn_tail_ptr, gpn_next_ptr, std::memory_order_seq_cst));

            gpn_tail_ptr->release_node();
            gpn_tail_ptr = gpn_next_ptr;
            gpn_next_ptr = gpn_tail_ptr->next_node.load(std::memory_order_seq_cst);
        }

        this->last_cleaned_head_ptr.store(current_head, std::memory_order_seq_cst);
        this->cleanup_head_ptr.store(NULL, std::memory_order_seq_cst);
    }
}
