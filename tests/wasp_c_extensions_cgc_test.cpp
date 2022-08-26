
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_cgc/cgc.hpp"


class TestWaspCGCTest:
    public CppUnit::TestFixture
{

    CPPUNIT_TEST_SUITE(TestWaspCGCTest);
    CPPUNIT_TEST(test);
    CPPUNIT_TEST_SUITE_END();

    void test(){
        wasp::cgc::ConcurrentGarbageCollector* cgc = new wasp::cgc::ConcurrentGarbageCollector();
        CPPUNIT_ASSERT(1 == 1);
        
        delete cgc;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( TestWaspCGCTest);
