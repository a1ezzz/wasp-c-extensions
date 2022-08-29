
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_pqueue/pqueue.hpp"

class TestWaspPQueueTest:
    public CppUnit::TestFixture
{

    CPPUNIT_TEST_SUITE(TestWaspPQueueTest);
    CPPUNIT_TEST(test);

    CPPUNIT_TEST_SUITE_END();

    void test(){
        wasp::pqueue::QueueItem item(1, NULL);
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(TestWaspPQueueTest);
