// wasp_c_extensions/_threads/event.hpp
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

#ifndef __WASP_C_EXTENSIONS__THREADS_EVENT_HPP__
#define __WASP_C_EXTENSIONS__THREADS_EVENT_HPP__

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>

namespace wasp::threads {

class Event {

    std::atomic<bool>         __is_set;
    std::condition_variable   conditional_variable;
    std::mutex                mutex;
    std::chrono::milliseconds max_timeout;

    public:

        Event(std::chrono::milliseconds t = std::chrono::milliseconds(-1));
        virtual ~Event();

        bool wait(std::chrono::milliseconds t = std::chrono::milliseconds(0));
        void clear();
        void set();
        bool is_set();
};

};  // namespace wasp::threads

#endif // __WASP_C_EXTENSIONS__THREADS_EVENT_HPP__
