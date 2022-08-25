// wasp_c_extensions/_pqueue/pqueue.hpp
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

#ifndef __WASP_C_EXTENSIONS__PQUEUE_PQUEUE_HPP__
#define __WASP_C_EXTENSIONS__PQUEUE_PQUEUE_HPP__

#include <atomic>
#include <cstddef>

namespace wasp::pqueue {

typedef long int item_priority;

class QueueItem {
    public:
        QueueItem(item_priority pr,const void* pa):
            priority(pr),
            payload(pa),
            dirty_marks(1),
            next(NULL),
            read_flag(false)
        {}
        virtual ~QueueItem(){};

        const item_priority priority;
        const void* payload;

        std::atomic<size_t> dirty_marks;
        std::atomic<QueueItem*> next;
        std::atomic<bool> read_flag;
};

class PriorityQueue{

    std::atomic<QueueItem*> read_ptr;
    std::atomic<QueueItem*> head;

    public:
        PriorityQueue();
        virtual ~PriorityQueue(){};

        virtual void push(QueueItem*);

        virtual QueueItem* next();

        virtual QueueItem* pull();
};

};  // namespace wasp::pqueue

#endif  //  __WASP_C_EXTENSIONS__PQUEUE_PQUEUE_HPP__
