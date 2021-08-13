// wasp_c_extensions/_ev_loop/ev_loop.cpp
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

#include "ev_loop.hpp"

using namespace wasp::ev_loop;

EventLoopBase::EventLoopBase(wasp::queue::ICMCQueue* q, std::chrono::milliseconds t, bool s):
    is_running(false),
    immediate_stop(s),
    queue(q),
    last_event(q->subscribe()),
    trigger(t)
{}

EventLoopBase::~EventLoopBase(){
    this->queue->unsubscribe(this->last_event);
}

void EventLoopBase::wait_event(){
    this->trigger.wait();
}

void EventLoopBase::start_loop(){
    bool is_running = this->is_running.test_and_set(std::memory_order_seq_cst);

    if (is_running){
        // TODO: raise something -- unable to start twice
        return;
    }

    while(this->is_running.test_and_set(std::memory_order_seq_cst)){
        if (! this->process_event()){
            this->wait_event();
        }
    }

    if (! this->immediate_stop){
        while (this->process_event());
    }

    this->is_running.clear(std::memory_order_seq_cst);
}

void EventLoopBase::stop_loop(){
    this->is_running.clear(std::memory_order_seq_cst);
    this->trigger.set();  // force loop to stop waiting
}
