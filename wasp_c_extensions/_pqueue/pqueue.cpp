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

QueueItem* QueueItem::create(
    wasp::cgc::ConcurrentGarbageCollector* gc, const item_priority priority, const void* payload
)
{
    QueueItem* item = NULL;
    assert(gc);

    item = new QueueItem(wasp::cgc::ConcurrentGCItem::heap_destroy_fn, priority, payload);
    gc->push(item);
    return item;
}

QueueItem::QueueItem(
    void (*destroy_fn)(PointerDestructor*),
    const item_priority pr,
    const void* pa
):
    wasp::cgc::ConcurrentGCItem(destroy_fn),
    priority(pr),
    payload(pa),
    read_flag(0),
    sorted_next(NULL),
    raw_next(NULL)
{}

QueueItem::~QueueItem(){
     assert(this->read_flag.load(std::memory_order_seq_cst) == true);
}

void QueueItem::gc_item_id(std::ostream& os){
    os << "QueueItem object [read_flag: " << this->read_flag.load(std::memory_order_seq_cst);
    os << ", priority: " << this->priority;
    os << ", payload: " << this->payload << "]";
}

PriorityQueue::PriorityQueue(wasp::cgc::ConcurrentGarbageCollector* _gc):
    gc(_gc),
    is_merge_and_cleanup_running(false),
    raw_head(NULL),
    // sorted_head(NULL),
    sorted_head(),
    cache_size_counter(0),
    queue_size_counter(0)
{
    QueueItem *dummy_head_item_ptr = NULL;
    smart_item_pointer *dummy_head_smart_ptr = NULL;

    assert(gc);

    dummy_head_item_ptr = QueueItem::create(this->gc, 0, this);
    dummy_head_item_ptr->read_flag.store(true, std::memory_order_seq_cst);

    dummy_head_smart_ptr = new smart_item_pointer(wasp::cgc::ConcurrentGCItem::heap_destroy_fn);
    assert(dummy_head_smart_ptr->init(dummy_head_item_ptr));
    this->gc->push(dummy_head_smart_ptr);

    this->sorted_head.setup_next(dummy_head_smart_ptr);
}

PriorityQueue::~PriorityQueue(){
    smart_item_pointer *head_sp_ptr = NULL;
    QueueItem *head_ptr = NULL;

    assert(this->write_flush());
    assert(! this->raw_head.load(std::memory_order_seq_cst));

    this->read_flush();
    this->cleanup(false);

    // this->gc->dump_items(std::cout);  // TODO: TEMP!!!

    head_sp_ptr = this->sorted_head.acquire_next();
    head_ptr = head_sp_ptr->acquire();
    assert(head_ptr);
    assert(head_ptr->read_flag.load(std::memory_order_seq_cst));
    head_sp_ptr->release();
    head_sp_ptr->release();
    head_sp_ptr->release();
    head_sp_ptr->destroyable();

    this->gc->collect();
}

void PriorityQueue::push(const item_priority priority, const void* payload){
    bool appended = false;
    QueueItem *head_ptr = NULL, *item_ptr = QueueItem::create(this->gc, priority, payload);

    do {
        head_ptr = this->raw_head.load(std::memory_order_seq_cst);
        item_ptr->raw_next.store(head_ptr, std::memory_order_seq_cst);
        appended = this->raw_head.compare_exchange_strong(head_ptr, item_ptr, std::memory_order_seq_cst);
    }
    while(! appended);

    this->cache_size_counter.fetch_add(1, std::memory_order_seq_cst);

    this->write_flush();
//    this->cleanup();
}

const void* PriorityQueue::pull(const bool probe){
    smart_item_pointer *prev_sp_ptr = NULL, *next_sp_ptr = NULL, *head_sp_ptr = NULL;
    QueueItem *item_sorted_ptr = NULL;
    bool false_value = false, item_found = false;
    const void* result = NULL;

    this->write_flush();

    while (! head_sp_ptr){
        head_sp_ptr = this->sorted_head.acquire_next(); // TODO: there is a concurrency issue between loading a head and acquire call
    }

    next_sp_ptr = head_sp_ptr;

    while (next_sp_ptr){
        item_sorted_ptr = next_sp_ptr->acquire();
        if (prev_sp_ptr){
            prev_sp_ptr->release();
            prev_sp_ptr = NULL;
        }
        assert(item_sorted_ptr);

        if (probe){
            item_found = (! item_sorted_ptr->read_flag.load(std::memory_order_seq_cst));
        }
        else {
            false_value = false;  // compare_exchange_strong may replace value
            item_found = item_sorted_ptr->read_flag.compare_exchange_strong(
                false_value, true, std::memory_order_seq_cst
            );
        }

        if (item_found) {
            result = item_sorted_ptr->payload;
            next_sp_ptr->release();
            if (! probe){
                this->queue_size_counter.fetch_sub(1, std::memory_order_seq_cst);
//                this->cleanup();
            }
            head_sp_ptr->release();
            return result;
        }

        prev_sp_ptr = next_sp_ptr;
        next_sp_ptr = item_sorted_ptr->sorted_next.load(std::memory_order_seq_cst);
    }

    head_sp_ptr->release();

    if (prev_sp_ptr){
        prev_sp_ptr->release();
        prev_sp_ptr = NULL;
    }

//    this->cleanup();
    return NULL;
}

bool PriorityQueue::write_flush(){
    QueueItem *detached_raw_head_ptr = NULL;
    bool is_mac_running = this->is_merge_and_cleanup_running.test_and_set(std::memory_order_seq_cst);

    if (is_mac_running){
        return false;
    }

    if (! this->sorted_head.next_replaceable()){
        this->is_merge_and_cleanup_running.clear(std::memory_order_seq_cst);
        return false;
    }

    detached_raw_head_ptr = this->detach_raw_head();
    if (detached_raw_head_ptr){
        this->merge_raw_head(detached_raw_head_ptr);
    }

    this->is_merge_and_cleanup_running.clear(std::memory_order_seq_cst);
    return true;
}

QueueItem* PriorityQueue::detach_raw_head() {
    QueueItem *head_ptr = NULL, *c_ptr = NULL;
    bool head_detached = false;

    do {
        head_ptr = this->raw_head.load(std::memory_order_seq_cst);
        if (head_ptr){
            head_detached = this->raw_head.compare_exchange_strong(head_ptr, NULL, std::memory_order_seq_cst);
        }
    }
    while(head_ptr && (!head_detached));

    c_ptr = head_ptr;

    while(c_ptr){
        c_ptr = c_ptr->raw_next.load(std::memory_order_seq_cst);
    }

    return head_detached ? head_ptr : NULL;
}

QueueItem* PriorityQueue::sort_raw_items(QueueItem* unsorted_head){
    QueueItem *prev_item = NULL, *parent_item = NULL, *item = NULL, *next_item = NULL, *sorted_head = NULL, *sorted_tail = NULL;
    item_priority item_priority, next_item_priority;

    while(unsorted_head){  // TODO: may be there is a better algorithm than O(n^2)

        item_priority = unsorted_head->priority;
        item = unsorted_head;
        prev_item = item;
        parent_item = item;
        next_item = item->raw_next.load(std::memory_order_seq_cst);

        while(next_item){

            next_item_priority = next_item->priority;
            if (next_item_priority > item_priority){  // TODO: reverse order!
                item_priority = next_item_priority;
                item = next_item;
                parent_item = prev_item;
            }

            prev_item = next_item;
            next_item = next_item->raw_next.load(std::memory_order_seq_cst);
        }

        if (unsorted_head == item){
            unsorted_head = unsorted_head->raw_next.load(std::memory_order_seq_cst);
        }
        else {
            assert(parent_item);
            parent_item->raw_next.store(
                item->raw_next.load(std::memory_order_seq_cst), std::memory_order_seq_cst
            );
        }

        item->raw_next.store(NULL, std::memory_order_seq_cst);

        if (sorted_tail){
            sorted_tail->raw_next.store(item, std::memory_order_seq_cst);
            sorted_tail = item;
        }
        else {
            sorted_head = item;
            sorted_tail = sorted_head;
        }
    }

    return sorted_head;
}

void PriorityQueue::merge_raw_head(QueueItem* raw_head) {
    smart_item_pointer *prev_prev_sp_ptr = NULL, *prev_sp_ptr = NULL, *item_sp_ptr = NULL, *null_sp_ptr = NULL, *next_sp_ptr = NULL;
    smart_item_pointer* head_sp_ptr = NULL;
    QueueItem *prev_sorted_ptr = NULL, *next_sorted_ptr = NULL;

    assert(raw_head);
    raw_head = this->sort_raw_items(raw_head);

    while(raw_head){

        item_sp_ptr = new smart_item_pointer(wasp::cgc::ConcurrentGCItem::heap_destroy_fn);
        assert(item_sp_ptr->init(raw_head));
        this->gc->push(item_sp_ptr);

        null_sp_ptr = NULL;

        head_sp_ptr = NULL;
        while (! head_sp_ptr){
            head_sp_ptr = this->sorted_head.acquire_next(); // TODO: this should be done only once, since original list is ordered
        }
        next_sp_ptr = head_sp_ptr;
        next_sorted_ptr = next_sp_ptr->acquire();
        assert(next_sorted_ptr);

        if (raw_head->priority > next_sorted_ptr->priority){  // TODO: reverse order!
            // replace a head
            raw_head->sorted_next.store(next_sp_ptr, std::memory_order_seq_cst);
            assert(this->sorted_head.setup_next(item_sp_ptr));
            next_sp_ptr->release();
        }
        else {

            while (next_sorted_ptr && (! (raw_head->priority > next_sorted_ptr->priority))) {  // TODO: reverse order!
                prev_prev_sp_ptr = prev_sp_ptr;
                prev_sp_ptr = next_sp_ptr;
                prev_sorted_ptr = next_sorted_ptr;
                next_sorted_ptr = NULL;
                next_sp_ptr = prev_sorted_ptr->sorted_next.load(std::memory_order_seq_cst);
                if (next_sp_ptr){
                    next_sorted_ptr = next_sp_ptr->acquire();
                    assert(next_sorted_ptr);

                    if (prev_prev_sp_ptr){
                        prev_prev_sp_ptr->release();
                        prev_prev_sp_ptr = NULL;
                    }
                }
            }

            raw_head->sorted_next.store(next_sp_ptr, std::memory_order_seq_cst);
            // prev_sorted_ptr->sorted_next.store(item_sp_ptr, std::memory_order_seq_cst);  // TODO: check what to choose store or compare_exchange?!
            assert(prev_sorted_ptr->sorted_next.compare_exchange_strong(next_sp_ptr, item_sp_ptr, std::memory_order_seq_cst));

            if (prev_prev_sp_ptr){
                prev_prev_sp_ptr->release();
                prev_prev_sp_ptr = NULL;
            }

            if (prev_sp_ptr){
                prev_sp_ptr->release();
                prev_sp_ptr = NULL;
            }

            if (next_sp_ptr){
                next_sp_ptr->release();
                next_sp_ptr = NULL;
            }

        } // else branch of the if (head->priority > next_sorted_ptr->priority)

        this->queue_size_counter.fetch_add(1, std::memory_order_seq_cst);
        this->cache_size_counter.fetch_sub(1, std::memory_order_seq_cst);

        raw_head = raw_head->raw_next.load(std::memory_order_seq_cst);

        head_sp_ptr->release();
    }  // while(raw_head)
}

void PriorityQueue::cleanup(bool run_gc) {
    smart_item_pointer *head_sp_ptr = NULL, *next_sp_ptr = NULL;
    QueueItem *head_sorted_ptr = NULL;
    bool items_deleted = false, is_mac_running = this->is_merge_and_cleanup_running.test_and_set(std::memory_order_seq_cst);

    if (is_mac_running){
        return;
    }

    head_sp_ptr = this->sorted_head.acquire_next();

    while(head_sp_ptr){
        head_sorted_ptr = head_sp_ptr->acquire();
        assert(head_sorted_ptr);

        if (head_sorted_ptr->read_flag.load(std::memory_order_seq_cst)){
            next_sp_ptr = head_sorted_ptr->sorted_next.load(std::memory_order_seq_cst);
            if (next_sp_ptr){

                if (this->sorted_head.setup_next(next_sp_ptr)){
                    head_sp_ptr->release();
                    head_sp_ptr->release();
                    head_sp_ptr->release();
                    head_sp_ptr->destroyable();
                    head_sp_ptr = this->sorted_head.acquire_next();

                    items_deleted = true;
                    continue;
                }
            }
        }

        head_sp_ptr->release();
        head_sp_ptr->release();
        break;
    }

    this->is_merge_and_cleanup_running.clear(std::memory_order_seq_cst);

    if (run_gc && items_deleted){
        this->gc->collect();
    }
}

void PriorityQueue::read_flush(){
    // TODO: do something
}

size_t PriorityQueue::cache_size(){
    return this->cache_size_counter.load(std::memory_order_seq_cst);
}

size_t PriorityQueue::queue_size(){
    return this->queue_size_counter.load(std::memory_order_seq_cst);
}

void PriorityQueue::dump(){
    QueueItem* item = NULL;
    smart_item_pointer *smart_ptr = NULL;

//    std::cout << " === QUEUE DUMP ===" << std::endl;
//
//    // smart_ptr = this->sorted_head.load(std::memory_order_seq_cst);
//    smart_ptr = this->sorted_head2.acquire();
//    assert(smart_ptr);
//    item = smart_ptr->acquire();
//    (assert(item));
//
//    while(item){
//        std::cout << "[[ queue ]] priority: " << item->priority << " payload: " << item->payload << std::endl;
//        smart_ptr = item->sorted_next.load(std::memory_order_seq_cst);
//        item = NULL;
//        if (smart_ptr){
//            item = smart_ptr->acquire();
//            (assert(item));
//        }
//    }
}
