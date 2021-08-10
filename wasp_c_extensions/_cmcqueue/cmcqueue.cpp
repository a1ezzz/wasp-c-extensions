// wasp_c_extensions/_queue/cmcqueue.cpp
//
//Copyright (C) 2020, 2021 the wasp-c-extensions authors and contributors
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

#include "cmcqueue.hpp"

using namespace wasp::queue;

// class StretchedBuffer

StretchedBuffer::StretchedBuffer():
    zero_node(new StretchedBufferItem(NULL)),
    tail(zero_node)
{}

StretchedBuffer::~StretchedBuffer(){
    // TODO: CLEAN UP ALL BUFFER!!!!
    // TODO: BE WARN ABOUT CONCURRENCY BETWEEN append/reduce and this destructor
    // TODO: assert that tail is NULL?!

    __WASP_DEBUG__("Buffer is about to be destroyed")

    delete this->zero_node;
}

const QueueItem* StretchedBuffer::append(QueueItem* i){

    StretchedBufferItem *tail_ptr = NULL, *item = dynamic_cast<StretchedBufferItem*>(i);

    if (! item){
        // TODO: do some bad things!
    }

    do {
        tail_ptr = this->tail.load(std::memory_order_seq_cst);
    }
    while (! this->tail.compare_exchange_strong(tail_ptr, item, std::memory_order_seq_cst));
    tail_ptr->next_item.store(item, std::memory_order_seq_cst);

    return item;
}

const QueueItem* StretchedBuffer::next(const QueueItem* i){
    // TODO: may be NULL !!! (note or throw something)
    const StretchedBufferItem* item_ptr = dynamic_cast<const StretchedBufferItem*>(i);
    return item_ptr->next_item.load(std::memory_order_seq_cst);
}

const QueueItem* StretchedBuffer::head(){
    return this->zero_node->next_item.load(std::memory_order_seq_cst);
}

const QueueItem* StretchedBuffer::reduce(){
    StretchedBufferItem *reduced_node_ptr, *next_ptr, *null_ptr = NULL;
    bool exchange_status;

    do {
        reduced_node_ptr = this->zero_node->next_item.load(std::memory_order_seq_cst);

        if (! reduced_node_ptr){
            return NULL;  // TODO: throw something. we asked to reduce, but nothing changed
        }

        next_ptr = reduced_node_ptr->next_item.load(std::memory_order_seq_cst);
        if (next_ptr){
            exchange_status = this->zero_node->next_item.compare_exchange_strong(
                reduced_node_ptr, next_ptr, std::memory_order_seq_cst
           );

           if (exchange_status) {
               return reduced_node_ptr;
           }
        }  // if next_ptr
        else {
            // if there is no next node, then it is a tail
            exchange_status = this->tail.compare_exchange_strong(
                reduced_node_ptr, this->zero_node, std::memory_order_seq_cst
            );

            if (exchange_status) {
                // tail points to a new node, but there may be a new 'append'

                this->zero_node->next_item.compare_exchange_strong(
                    reduced_node_ptr, null_ptr, std::memory_order_seq_cst
                );

                return reduced_node_ptr;
            }
        }
    }
    while (true);
}

// class CMCBaseQueue

CMCBaseQueue::CMCBaseQueue(IQueueBuffer* b):
    buffer(b),
    oldest_subscribes(0),
    newest_subscribes(0),
    __messages(0),
    is_cleaning(false)
{}

CMCBaseQueue::~CMCBaseQueue()
{
    __WASP_DEBUG__("BaseQueue is about to be destroyed")

#ifdef __WASP_DEBUG_ENABLED__
    if (this->newest_subscribes.load(std::memory_order_seq_cst)){
            __WASP_DEBUG__("Warning! This queue has subscribers still! External pointers will corrupt");
    }
#endif

    const QueueItem* next_item_ptr = this->buffer->head();
    CMCQueueItem* item_ptr;

    while (next_item_ptr){
        item_ptr = this->cast_item(next_item_ptr);
        this->buffer->reduce();
        delete item_ptr;
        next_item_ptr = this->buffer->head();
    }
}

CMCQueueItem* CMCBaseQueue::cast_item(const QueueItem* item){
    CMCQueueItem* casted_item = const_cast<CMCQueueItem*>(dynamic_cast<const CMCQueueItem*>(item));
    if (item && (!casted_item)){
        // TODO: do something bad!
    }
    return casted_item;
}

CMCQueueItem* CMCBaseQueue::push_(const void* payload, CMCItemType type){
    CMCQueueItem* item_ptr = this->queue_item(payload, type);
    item_ptr->reads.store(0, std::memory_order_seq_cst);  // force, because of untrusted source
    this->buffer->append(item_ptr);
    this->__messages.fetch_add(1, std::memory_order_seq_cst);
    return item_ptr;
}

CMCQueueItem* CMCBaseQueue::pull_(const QueueItem* last_item)
{
    CMCQueueItem* prev_item_ptr = this->cast_item(last_item);
    const QueueItem* next_item_ptr = this->buffer->next(last_item);

    if (next_item_ptr){
        prev_item_ptr->reads.fetch_add(1, std::memory_order_seq_cst);
        this->cleanup();
    }
    return this->cast_item(next_item_ptr);
};

const QueueItem* CMCBaseQueue::push(const void* payload){
    if (! payload){
        // TODO: do something bad
        return NULL;
    }

    if (! this->newest_subscribes.load(std::memory_order_seq_cst)){
        // there is no subscribers
        return NULL;
    }

    const QueueItem* result = this->push_(payload, MSG_USERS_PAYLOAD);
    return result;
}

const QueueItem* CMCBaseQueue::subscribe(){
    __WASP_DEBUG__("Subscribe to a queue");
    this->newest_subscribes.fetch_add(1, std::memory_order_seq_cst);
    const QueueItem* result = this->push_(NULL, MSG_CMD_SUBSCRIPTION);
    return result;
}

void CMCBaseQueue::unsubscribe(const QueueItem* latest_read_ptr){
    __WASP_DEBUG__("Unsubscribe from a queue");

    CMCQueueItem* unsubscribe_item_ptr = this->push_(NULL, MSG_CMD_UNSUBSCRIPTION);
    CMCQueueItem* item_ptr;

    do {
        item_ptr = this->cast_item(latest_read_ptr);
        item_ptr->reads.fetch_add(1, std::memory_order_seq_cst);
        latest_read_ptr = this->buffer->next(latest_read_ptr);
    }
    while (latest_read_ptr && latest_read_ptr != unsubscribe_item_ptr);

    if (! latest_read_ptr){
        // end is reached. Queue is corrupted
        // TODO: throw something
    }

    unsubscribe_item_ptr->reads.fetch_add(1, std::memory_order_seq_cst);
    this->newest_subscribes.fetch_sub(1, std::memory_order_seq_cst);

    this->cleanup();
}

void CMCBaseQueue::cleanup(){
    __WASP_DEBUG__("Cleaning a queue");

    const QueueItem* next_item_ptr;
    CMCQueueItem* item_ptr;
    size_t reads = 0, subscribers = 0;
    bool is_dirty = true, is_cleaning = this->is_cleaning.test_and_set(std::memory_order_seq_cst);

    if (is_cleaning){
        return;
    }

    next_item_ptr = this->buffer->head();
    subscribers = this->oldest_subscribes.load(std::memory_order_seq_cst);

    while (next_item_ptr){
        item_ptr = this->cast_item(next_item_ptr);
        reads = item_ptr->reads.load(std::memory_order_seq_cst);

        switch (item_ptr->type){
            case MSG_USERS_PAYLOAD:
                is_dirty = (reads < subscribers);
                break;
            case MSG_CMD_SUBSCRIPTION:
                is_dirty = (reads <= subscribers);
                if (! is_dirty){
                    this->oldest_subscribes.fetch_add(1, std::memory_order_seq_cst);
                    subscribers++;
                }
                break;
            case MSG_CMD_UNSUBSCRIPTION:
                is_dirty = ((reads + 1) < subscribers);
                if (! is_dirty){
                    this->oldest_subscribes.fetch_sub(1, std::memory_order_seq_cst);
                    subscribers--;
                }
                break;
            default:
                // TODO: throw exception for unhandled type
                break;
        };

        if (is_dirty){
            break;
        }

        this->buffer->reduce();
        this->__messages.fetch_sub(1, std::memory_order_seq_cst);
        delete item_ptr;
        next_item_ptr = this->buffer->head();
    } // while (next_item_ptr)

    this->is_cleaning.clear(std::memory_order_seq_cst);
}

const QueueItem* CMCBaseQueue::pull(const QueueItem* last_item)
{
    CMCQueueItem* next_item = this->pull_(last_item);
    while (next_item && next_item->type != MSG_USERS_PAYLOAD){
        last_item = next_item;
        next_item = this->pull_(last_item);
    }

    return next_item ? next_item : last_item;
}

bool CMCBaseQueue::has_next(const QueueItem* item){
    return (this->buffer->next(item) != NULL);
}

size_t CMCBaseQueue::messages(){
    return this->__messages.load(std::memory_order_seq_cst);
}
