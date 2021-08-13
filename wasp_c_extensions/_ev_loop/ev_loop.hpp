// wasp_c_extensions/_ev_loop/ev_loop.hpp
//
//Copyright (C) 2021 the wasp-c-extensions authors and contributors
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

#ifndef __WASP_C_EXTENSIONS__EV_LOOP_EV_LOOP_HPP__
#define __WASP_C_EXTENSIONS__EV_LOOP_EV_LOOP_HPP__

#include "_cmcqueue/cmcqueue.hpp"
#include "_threads/event.hpp"

namespace wasp::ev_loop {

class EventLoopBase{
    std::atomic_flag is_running;
    bool             immediate_stop;

    protected:
        wasp::queue::ICMCQueue* queue;  // TODO: multiple queues with different priorities!
        const wasp::queue::QueueItem* last_event;
        wasp::threads::Event trigger;

    public:
        EventLoopBase(
            wasp::queue::ICMCQueue*,
            std::chrono::milliseconds t = std::chrono::milliseconds(-1),
            bool immediate_stop=true
        );
        virtual ~EventLoopBase();

        virtual bool process_event() = 0;
        virtual void wait_event();

        virtual void start_loop();
        virtual void stop_loop();
};

class IEventCallback{
    public:
        virtual ~IEventCallback(){};
        virtual void operator()() = 0;
};

template<typename T=IEventCallback>
class EventLoop:
    public EventLoopBase
{
    protected:
        virtual void call(T* callback){
            (*callback)();
        }

        virtual void notify_impl(T* callback){
            this->queue->push(callback);
            this->trigger.set();
        }

    public:

        EventLoop(
            wasp::queue::ICMCQueue* q,
            std::chrono::milliseconds t = std::chrono::milliseconds(-1),
            bool immediate_stop=true
        ):
            EventLoopBase(q, t, immediate_stop)
        {};

        virtual ~EventLoop(){};

        virtual void notify(T* callback){
            this->notify_impl(callback);
        }

        bool process_event(){
            const wasp::queue::QueueItem* next_event = this->queue->pull(this->last_event);
            if ((!next_event) || (next_event == this->last_event)){
                this->trigger.clear();
                if (this->queue->has_next(this->last_event)){
                    return this->process_event();  // one more try
                }
                return false;
            }

            if (next_event->payload){
                this->call(
                    const_cast<T*>(
                            static_cast<const T*>(next_event->payload)
                        )
                );
            }

            this->last_event = next_event;
            return true;
        }

        void wait_event(){
            EventLoopBase::wait_event();
        }
};

};  // namespace wasp::ev_loop

#endif  // __WASP_C_EXTENSIONS__EV_LOOP_EV_LOOP_HPP__
