
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
    CPPUNIT_TEST(test_item_destroy);
    CPPUNIT_TEST(test_item);
    CPPUNIT_TEST(test_gc_push);
    CPPUNIT_TEST_SUITE_END();

    void test_item_destroy(){
        CPPUNIT_ASSERT_THROW(wasp::cgc::ConcurrentGCItem::destroy(NULL), wasp::cgc::NullPointerException);
    }

    void test_item(){
        Item* item_ptr = Item::create();

        CPPUNIT_ASSERT_THROW(new Item(NULL), wasp::cgc::NullPointerException);

        CPPUNIT_ASSERT_EQUAL(item_ptr->gc_ready.load(std::memory_order_seq_cst), false);
        CPPUNIT_ASSERT(item_ptr->next.load(std::memory_order_seq_cst) == NULL);

        item_ptr->gc_ready.store(true, std::memory_order_seq_cst);  // without it the assertion fails

        wasp::cgc::ConcurrentGCItem::destroy(item_ptr);
    }

    void test_gc_push(){
        Item* item_ptr = NULL;
        wasp::cgc::ConcurrentGarbageCollector* cgc = new wasp::cgc::ConcurrentGarbageCollector();

        CPPUNIT_ASSERT_THROW(cgc->push(NULL), wasp::cgc::NullPointerException);

        item_ptr = Item::create();
        item_ptr->gc_ready.store(true, std::memory_order_seq_cst);  // without it the assertion fails
        CPPUNIT_ASSERT_THROW(cgc->push(item_ptr), wasp::cgc::InvalidItemState);

        item_ptr->gc_ready.store(false, std::memory_order_seq_cst);  // only a test may revert this flag
        // do not do this in a real code
        cgc->push(item_ptr);

        item_ptr->gc_ready.store(true, std::memory_order_seq_cst);  // without gc would not be
        cgc->collect();

        delete cgc;  // collection happens on destruction
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( TestWaspCGCTest);
