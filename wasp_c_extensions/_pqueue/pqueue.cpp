// wasp_c_extensions/_pqueue/pqueue.cpp
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

#include "pqueue.hpp"

using namespace wasp::pqueue;
using namespace wasp::cgc;

QueueItem::QueueItem(
    void (*destroy_fn)(PointerDestructor*),
    const item_priority pr,
    const void* pa,
    wasp::cgc::CGCSmartPointer<QueueItem>* sp
):
    wasp::cgc::ConcurrentGCItem(destroy_fn),
    priority(pr),
    payload(pa),
    next_item(NULL),
    read_flag(false),
    smart_pointer(sp)
{
    assert(sp->replace(this));
}

QueueItem::~QueueItem(){
    assert(this->read_flag == true);
}

QueueItem* QueueItem::create(
    wasp::cgc::ConcurrentGarbageCollector* gc, const item_priority priority, const void* payload
)
{
    QueueItem* result = NULL;
    wasp::cgc::CGCSmartPointer<QueueItem>* smart_pointer = new wasp::cgc::CGCSmartPointer<QueueItem>(
        wasp::cgc::ConcurrentGCItem::heap_destroy_fn
    );

    assert(gc);

    result = new QueueItem(wasp::cgc::ConcurrentGCItem::heap_destroy_fn, priority, payload, smart_pointer);
    gc->push(result);
    gc->push(smart_pointer);
    return result;
}

void QueueItem::gc_item_id(std::ostream& os){
    os << "QueueItem object [read_flag: " << this->read_flag.load(std::memory_order_seq_cst);
    os << ", priority: " << this->priority;
    os << ", payload: " << this->payload << "]";
}

PriorityQueue::PriorityQueue(wasp::cgc::ConcurrentGarbageCollector* _gc):
    gc(_gc),
    head(new wasp::cgc::CGCSmartPointer<QueueItem>(wasp::cgc::ConcurrentGCItem::heap_destroy_fn)),
    cleanup_running(false)
{
    assert(gc);
    gc->push(this->head);
}

PriorityQueue::~PriorityQueue(){
    wasp::cgc::CGCSmartPointer<QueueItem> *head_ptr = this->head.load(std::memory_order_seq_cst);

    QueueItem *item_ptr = dynamic_cast<QueueItem*>(head_ptr->acquire());

    if (item_ptr){
        this->cleanup(false);

        // there may be a single item maximum
        assert(item_ptr->read_flag.load(std::memory_order_seq_cst));
        assert(item_ptr->next_item.load(std::memory_order_seq_cst) == NULL);

        item_ptr->smart_pointer->block_mode();
        item_ptr->smart_pointer->destroyable();
        item_ptr->smart_pointer->release();

        head_ptr->release();  // release the "acquire" from this destructor
    }

    head_ptr->block_mode();
    head_ptr->destroyable();
    this->gc->collect();
}

QueueItem* PriorityQueue::acquire_head_for_push(QueueItem* item_ptr){
    wasp::cgc::CGCSmartPointer<QueueItem> *head_smart_ptr = NULL;
    QueueItem* current_head_ptr = NULL;
    bool head_found = false;

    assert(item_ptr);

    while (!head_found){

        if (current_head_ptr){
            current_head_ptr->smart_pointer->release();
        }

        head_smart_ptr = this->head.load(std::memory_order_seq_cst);
        current_head_ptr = head_smart_ptr->acquire();

        if (!current_head_ptr){
            if (this->head.compare_exchange_strong(head_smart_ptr, item_ptr->smart_pointer, std::memory_order_seq_cst)){
                head_smart_ptr->block_mode();
                head_smart_ptr->destroyable();
                return item_ptr;
            }
            continue;  // one more try
        }

        if (current_head_ptr->priority >= item_ptr->priority){  // TODO: optional order
            head_found = true;
        }
        else {
            item_ptr->next_item.store(current_head_ptr, std::memory_order_seq_cst);

            if (this->head.compare_exchange_strong(head_smart_ptr, item_ptr->smart_pointer, std::memory_order_seq_cst)){
                head_smart_ptr->release();
                return item_ptr;
            }
        }
    }

    return current_head_ptr;
}

void PriorityQueue::push(const item_priority priority, const void* payload){
    QueueItem *head_ptr = NULL, *null_ptr = NULL, *item_ptr = NULL;
    PriorityQueue::priority_pair priority_pair;

    item_ptr = QueueItem::create(this->gc, priority, payload);

    head_ptr = this->acquire_head_for_push(item_ptr);
    if(head_ptr == item_ptr){
        return;
    }

    // there is a head
    assert(head_ptr);

    priority_pair = std::make_pair(head_ptr, null_ptr);

    do {
        priority_pair = this->search_next(priority_pair.first, priority);  // the second may be null
        item_ptr->next_item.store(priority_pair.second, std::memory_order_seq_cst);
    }
    while (! priority_pair.first->next_item.compare_exchange_strong(
        priority_pair.second, item_ptr, std::memory_order_seq_cst
    ));

    head_ptr->smart_pointer->release();
//    this->cleanup(true);
}

PriorityQueue::priority_pair PriorityQueue::search_next(QueueItem* item, const item_priority priority){

    assert(item);

    QueueItem* priority_next = item;

    do {
        item = priority_next;
        priority_next = item->next_item.load(std::memory_order_seq_cst);
        if (priority_next && priority_next->priority < priority){  // TODO: optional order
            return std::make_pair(item, priority_next);
        }
    }
    while(priority_next);

    return std::make_pair(item, priority_next);
}

const item_priority PriorityQueue::next(item_priority default_value){
    wasp::cgc::CGCSmartPointer<QueueItem> *head_ptr = this->head.load(std::memory_order_seq_cst);
    QueueItem *next_ptr = dynamic_cast<QueueItem*>(head_ptr->acquire());
    bool release = false;
    bool item_found = false;
    item_priority result;

    while((! item_found) && next_ptr){
        release = true;
        if (! next_ptr->read_flag.load(std::memory_order_seq_cst)){
            item_found = true;
            result = next_ptr->priority;
        }
        else {
            next_ptr = next_ptr->next_item.load(std::memory_order_seq_cst);
        }
    }

    if (release){
        head_ptr->release();
    }

//    this->cleanup(true);
    return item_found ? result : default_value;
}

const void* PriorityQueue::pull(){
    wasp::cgc::CGCSmartPointer<QueueItem> *head_ptr = this->head.load(std::memory_order_seq_cst);
    QueueItem *next_ptr = dynamic_cast<QueueItem*>(head_ptr->acquire());
    bool item_found = false;
    bool release = false;
    const void* result = NULL;

    while((! item_found) && next_ptr){
        release = true;
        if (! next_ptr->read_flag.exchange(true, std::memory_order_seq_cst)){
            item_found = true;
            result = next_ptr->payload;
        }
        else {
            next_ptr = next_ptr->next_item.load(std::memory_order_seq_cst);
        }
    }

    if (release){
        head_ptr->release();
    }
//    this->cleanup(true);
    return result;
}

void PriorityQueue::cleanup(bool call_gc){
    while (this->cleanup_head());
    if (call_gc){
        this->gc->collect();
    }
}

bool PriorityQueue::cleanup_head(){
    wasp::cgc::CGCSmartPointer<QueueItem> *head_ptr = this->head.load(std::memory_order_seq_cst);
    QueueItem *item_ptr = NULL, *next_ptr = NULL, *next_after_next_ptr = NULL;
    bool result = false;

    if (this->cleanup_running.compare_exchange_strong(result, true, std::memory_order_seq_cst)){

        item_ptr = dynamic_cast<QueueItem*>(head_ptr->acquire());

        if (item_ptr->read_flag.load(std::memory_order_seq_cst)){
            next_ptr = item_ptr->next_item;
            if (next_ptr && next_ptr->read_flag.load(std::memory_order_seq_cst)){

                next_after_next_ptr = next_ptr->next_item;  // TODO: next item of the next_ptr may be in change right now!
                result = item_ptr->next_item.compare_exchange_strong(next_ptr, next_after_next_ptr);

                if (result){
                    next_ptr->smart_pointer->block_mode();
                    next_ptr->smart_pointer->destroyable();
                    next_ptr->smart_pointer->release();
                }
            };
        }
        head_ptr->release();
        this->cleanup_running.store(false, std::memory_order_seq_cst);
    }

    return result;
}
