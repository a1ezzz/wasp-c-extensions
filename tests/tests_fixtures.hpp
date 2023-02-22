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
#include <map>
#include <thread>

#include <cppunit/TestCase.h>

namespace wasp::tests_fixtures {

class ThreadsRunner:
    public CppUnit::TestFixture
{
    typedef std::vector<std::thread*> threads_vector;
    typedef std::map<std::string, threads_vector> tagged_threads;
    tagged_threads threads;

    public:
        void start_threads(std::string tag, size_t count, void (*threaded_fn)());
        void join_threads(std::string tag);
};

};  // wasp::tests_fixtures
