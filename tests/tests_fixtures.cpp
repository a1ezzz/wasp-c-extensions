// tests/tests_fixtures.cpp
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

#include "tests_fixtures.hpp"

using namespace wasp::tests_fixtures;

template<>
std::list<size_t> wasp::tests_fixtures::sequence_generator<0>() {
    return {};
};

void ThreadsRunner::start_threads(std::string tag, size_t count, std::function<void()>threaded_fn, bool delayed_start){
    std::atomic<bool>* flag = NULL;
    std::mutex* mutex = NULL;
    std::condition_variable* cv = NULL;
    threads_vector _threads;
    tagged_threads::iterator it = this->threads.find(tag);
    std::function<void()> wrapped_function = threaded_fn;

    if (it != this->threads.end()){
        throw std::invalid_argument("The specified threads are started already");
    }

    if (delayed_start){
        flag = new std::atomic<bool>(false);
        mutex = new std::mutex();
        cv = new std::condition_variable();

        wrapped_function = [&flag, &mutex, &cv, &threaded_fn](){
            while (! flag->load(std::memory_order_seq_cst)){
                std::unique_lock<std::mutex> lock(*mutex);
                cv->wait(lock);
            }
            threaded_fn();
        };
    }

    for (size_t i = 0; i < count; i++){
        _threads.push_back(new std::thread(threaded_fn));
    }

    this->threads.insert(std::make_pair(tag, threads_tuple(_threads, flag, mutex, cv)));
};

void ThreadsRunner::resume_threads(std::string tag){
    tagged_threads::iterator it = this->threads.find(tag);

    if (it == this->threads.end()){
        throw std::invalid_argument("The specified threads hasn't been started yet");
    }

    if (! this->get_item<1>(it)){
        throw std::invalid_argument("The specified threads doesn't await");
    }

    {
        this->get_item<1>(it)->store(true, std::memory_order_seq_cst);
        std::lock_guard<std::mutex> lock(*(this->get_item<2>(it)));
        this->get_item<3>(it)->notify_all();
    }
}

void ThreadsRunner::join_threads(std::string tag){
    tagged_threads::iterator it = this->threads.find(tag);

    if (it == this->threads.end()){
        throw std::invalid_argument("The specified threads hasn't been started yet");
    }

    for (size_t i = 0; i < this->get_item<0>(it).size(); i++){
        this->get_item<0>(it)[i]->join();
        delete this->get_item<0>(it)[i];
    }

    if (this->get_item<1>(it)){
        delete this->get_item<1>(it);
    }

    if (this->get_item<2>(it)){
        delete this->get_item<2>(it);
    }

    if (this->get_item<3>(it)){
        delete this->get_item<3>(it);
    }

    this->threads.erase(it);
};
