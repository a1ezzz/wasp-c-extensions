// wasp_c_extensions/_threads/event.cpp
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

#include "event.hpp"

using namespace wasp::threads;

Event::Event(std::chrono::milliseconds t):
    __is_set(false),
    conditional_variable(),
    mutex(),
    max_timeout(t)
{}
Event::~Event(){}

void Event::clear(){
    this->__is_set.store(false, std::memory_order_seq_cst);
}

void Event::set(){
    bool true_v = true, false_v = false;

    if (this->__is_set.compare_exchange_strong(false_v, true_v, std::memory_order_seq_cst)){
        std::unique_lock<std::mutex> lock(this->mutex);
        this->conditional_variable.notify_all();
    }
}

bool Event::is_set(){
    return this->__is_set.load(std::memory_order_seq_cst);
}

bool Event::wait(std::chrono::milliseconds wait_timeout)
{
    // if t == 0: just check and do not wait
    // if t < 0: use max_timeout (or if max_timeout isn't set wait forever)
    // if t > 0: check max_timeout and if it is defined use the lowest value

    std::chrono::milliseconds zero_timeout(0);

    std::unique_lock<std::mutex> lock(this->mutex);

    if (
        (this->max_timeout > zero_timeout) &&
        ((wait_timeout <= zero_timeout) || (this->max_timeout < wait_timeout))
    ){
        wait_timeout = this->max_timeout;
    }

    if (this->is_set()){
        return true;
    }

    if (wait_timeout < zero_timeout){
        this->conditional_variable.wait(lock);
    }
    else if (wait_timeout > zero_timeout) {
        this->conditional_variable.wait_for(lock, wait_timeout);
    }

    return this->is_set();
}
