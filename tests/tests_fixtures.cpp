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

void ThreadsRunner::start_threads(std::string tag, size_t count, void (*threaded_fn)()){
    threads_vector _threads;
    tagged_threads::iterator it = this->threads.find(tag);

    if (it != this->threads.end()){
        throw std::invalid_argument("The specified threads are started already");
    }

    for (size_t i = 0; i < count; i++){
        _threads.push_back(new std::thread(threaded_fn));
    }

    this->threads.insert(std::make_pair(tag, _threads));
};

void ThreadsRunner::join_threads(std::string tag){
    tagged_threads::iterator it = this->threads.find(tag);

    if (it == this->threads.end()){
        throw std::invalid_argument("The specified threads hasn't been started yet");
    }

    for (size_t i = 0; i < it->second.size(); i++){
        it->second[i]->join();
        delete it->second[i];
    }

    this->threads.erase(it);
};
