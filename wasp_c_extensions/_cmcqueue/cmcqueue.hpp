// wasp_c_extensions/_queue/cmcqueue.hpp
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

#ifndef __WASP_C_EXTENSIONS__CMCQUEUE_CMCQUEUE_HPP__
#define __WASP_C_EXTENSIONS__CMCQUEUE_CMCQUEUE_HPP__

#include <atomic>
#include <cstddef>
#include <cstdio>

#include "common.h"

namespace wasp::queue {

class QueueItem {
    public:
        QueueItem(const void* p):
            payload(p)
        {}
        virtual ~QueueItem(){};

        const void* payload;
};

class IQueueBuffer {
    public:
        virtual ~IQueueBuffer(){};
        virtual const QueueItem* append(QueueItem*) = 0;  // thread-safe method that store new payload in this buffer.
        // a newly appended item is returned;
        virtual const QueueItem* next(const QueueItem*) = 0;  // thread-safe method that return next item after the
        // specified one. If there are no such item -- return 0;
        virtual const QueueItem* reduce() = 0;  // thread-safe method that remove the oldest item from this buffer
        // and return it
        virtual const QueueItem* head() = 0;  // a head of a queue (the oldest item)
};

template <typename T> class ITypedQueueBuffer:
    public IQueueBuffer
{
    static_assert(std::is_base_of<QueueItem, T>::value, "T must extend QueueItem");

    public:
        typedef T BufferItem;
        virtual ~ITypedQueueBuffer(){};
};

class StretchedBufferItem:
    virtual public QueueItem
{
    public:
        StretchedBufferItem(const void* p):
            QueueItem(p),
            next_item(NULL)
        {}
        std::atomic<StretchedBufferItem*> next_item;
        virtual ~StretchedBufferItem(){};
};

class StretchedBuffer:
    public ITypedQueueBuffer<StretchedBufferItem>
{
    // TODO: somewhere should be a note that this simple implementation may lead to OOM when items are pushed faster
    // than pulled

    StretchedBufferItem* zero_node; // TODO -- helps to clean up
    std::atomic<StretchedBufferItem*> tail;

    public:
        StretchedBuffer();
        virtual ~StretchedBuffer();
        virtual const QueueItem* append(QueueItem*);  // the 'IQueueBuffer::append' implementation
        virtual const QueueItem* next(const QueueItem*);  // the 'IQueueBuffer::next' implementation
        virtual const QueueItem* reduce();  // the 'IQueueBuffer::reduce' implementation
        virtual const QueueItem* head();
};

//class QueueRingBuffer:
//    public IQueueBuffer
//{
//    std::size_t ring_size;
//
//    public:
//        QueueRingBuffer(std::size_t);
//};

enum CMCItemType {
    MSG_USERS_PAYLOAD = 0,
    MSG_CMD_SUBSCRIPTION = 1,
    MSG_CMD_UNSUBSCRIPTION = 2
};

class CMCQueueItem:
    virtual public QueueItem
{
    public:
        const CMCItemType type;
        std::atomic<size_t> reads;

        CMCQueueItem(const void* p, const CMCItemType t):
            QueueItem(p),
            type(t),
            reads(0)
        {}
        virtual ~CMCQueueItem(){};
};


class ICMCQueue{
    public:
        virtual ~ICMCQueue(){};

        virtual const QueueItem* push(const void* payload) = 0; // TODO: add note that NULL as payload is prohibbited
        virtual const QueueItem* pull(const QueueItem*) = 0;
        virtual bool has_next(const QueueItem*) = 0;
        virtual const QueueItem* subscribe() = 0;
        virtual void unsubscribe(const QueueItem*) = 0;

        virtual size_t messages() = 0;
};

class CMCBaseQueue:
    public ICMCQueue
{

    IQueueBuffer* buffer;

    std::atomic<size_t> oldest_subscribes;  // counter for cleaning
    std::atomic<size_t> newest_subscribes;  // counter for push optimization
    std::atomic<size_t> __messages;  // approximate message counter
    std::atomic_flag is_cleaning;

    CMCQueueItem* cast_item(const QueueItem*);
    CMCQueueItem* push_(const void* payload, CMCItemType t=MSG_USERS_PAYLOAD);
    CMCQueueItem* pull_(const QueueItem*);
    void cleanup();

    protected:
        virtual CMCQueueItem* queue_item(const void* payload, CMCItemType type) = 0;

    public:
        CMCBaseQueue(IQueueBuffer* buffer);
        virtual ~CMCBaseQueue();

        const QueueItem* push(const void* payload); // TODO: add note that NULL as payload is prohibbited
        const QueueItem* pull(const QueueItem*);
        bool has_next(const QueueItem*);

        const QueueItem* subscribe();
        void unsubscribe(const QueueItem*);

        size_t messages();
};

inline static void dummy_item_cleanup_function(QueueItem*){}

template<typename T, void (*F)(QueueItem*) = dummy_item_cleanup_function> class CMCQueue:
    private T,
    public CMCBaseQueue
{
    static_assert(
        std::is_base_of<ITypedQueueBuffer<typename T::BufferItem>, T>::value, "T must extend ITypedQueueBuffer"
    );

    class Item:
        public T::BufferItem,
        public CMCQueueItem
    {
        std::atomic<size_t> reads;

        public:
            Item(const void* payload, CMCItemType type):
                QueueItem(payload),
                T::BufferItem(payload),
                CMCQueueItem(payload, type)
            {}

            virtual ~Item(){
                if (this->type == MSG_USERS_PAYLOAD){
                    F(this);
                }
            }
    };

    protected:
        virtual CMCQueueItem* queue_item(const void* payload, CMCItemType type){
            return new CMCQueue<T, F>::Item(payload, type);
        }

    public:
        CMCQueue():
            T(),
            CMCBaseQueue(this)
        {};
        virtual ~CMCQueue(){};
};

};  // namespace wasp::queue

#endif // __WASP_C_EXTENSIONS__CMCQUEUE_CMCQUEUE_HPP__
