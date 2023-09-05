// tests/tests_fixtures.hpp
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

#include <vector>
#include <atomic>
#include <map>
#include <list>
#include <thread>
#include <functional>
#include <condition_variable>
#include <mutex>

#include <cppunit/TestCase.h>

#include "wasp_c_extensions/_cgc/cgc.hpp"

namespace wasp::tests_fixtures {

template<size_t count>
std::list<size_t> sequence_generator(){
    std::list<size_t> result = sequence_generator<count - 1>();
    result.push_back(count);
    return result;
};

template<>
std::list<size_t> sequence_generator<0>();

class ThreadsRunner
{
    typedef std::vector<std::thread*> threads_vector;
    typedef std::tuple<threads_vector, std::atomic<bool>*, std::mutex*, std::condition_variable*> threads_tuple;
    typedef std::map<std::string, threads_tuple> tagged_threads;
    tagged_threads threads;

    template<int a>
    auto get_item(tagged_threads::iterator it){
        return std::get<a>(it->second);
    };

    public:
        void start_threads(std::string tag, size_t count, std::function<void()>threaded_fn, bool delayed_start=false);
        void resume_threads(std::string tag);
        void join_threads(std::string tag);
};

class ThreadsRunnerFixture:
    public ThreadsRunner,
    public CppUnit::TestFixture
{};

class GCRunner
{
    wasp::cgc::ConcurrentGarbageCollector* cgc;

    public:

        GCRunner();
        virtual ~GCRunner();

        wasp::cgc::ConcurrentGarbageCollector* collector();

        void setUp();
        void tearDown();
};

class GCRunnerFixture:
    public GCRunner,
    public CppUnit::TestFixture
{
    public:
    void setUp(){GCRunner::setUp();};
    void tearDown(){GCRunner::tearDown();};
};

class GCThreadsRunnerFixture:
    public ThreadsRunner,
    public GCRunner,
    public CppUnit::TestFixture
{
    public:
    void setUp(){GCRunner::setUp();};
    void tearDown(){GCRunner::tearDown();};
};

};  // wasp::tests_fixtures
