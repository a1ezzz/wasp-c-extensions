
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_cgc/cgc.hpp"


class TestWaspCGCTest:
    public CppUnit::TestFixture
{

    class Item:
        public wasp::cgc::ConcurrentGCItem
    {

        public:
            Item(void (*fn)(ConcurrentGCItem*)):
                wasp::cgc::ConcurrentGCItem(fn)
            {}

            static Item* create(){
                return new Item(&Item::destroy);
            }

            static void destroy(wasp::cgc::ConcurrentGCItem* ptr){
                delete ptr;
            }
    };

    CPPUNIT_TEST_SUITE(TestWaspCGCTest);
    CPPUNIT_TEST(test_item);
    CPPUNIT_TEST(test);
    CPPUNIT_TEST_SUITE_END();

    void test_item(){
        Item* item_ptr = Item::create();
        void (*destroy_fn)(wasp::cgc::ConcurrentGCItem*) = item_ptr->destroy_fn;

        CPPUNIT_ASSERT_EQUAL(item_ptr->gc_ready.load(std::memory_order_seq_cst), false);
        CPPUNIT_ASSERT(item_ptr->next.load(std::memory_order_seq_cst) == NULL);
        CPPUNIT_ASSERT(item_ptr->prev.load(std::memory_order_seq_cst) == NULL);

        destroy_fn(item_ptr);
    }

    void test(){
        wasp::cgc::ConcurrentGarbageCollector* cgc = new wasp::cgc::ConcurrentGarbageCollector();
        CPPUNIT_ASSERT(1 == 1);
        
        delete cgc;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( TestWaspCGCTest);
