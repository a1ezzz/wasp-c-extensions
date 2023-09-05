
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_cgc/cgc.hpp"
#include "wasp_c_extensions/_cgc/smart_ptr.hpp"

#include "tests/tests_fixtures.hpp"

namespace wasp::cgc_test_case {

class TestWaspCGCTest:
    public wasp::tests_fixtures::ThreadsRunnerFixture
{

    CPPUNIT_TEST_SUITE(TestWaspCGCTest);
    CPPUNIT_TEST(test_item_destroy);
    CPPUNIT_TEST(test_item);
    CPPUNIT_TEST(test_gc);
    CPPUNIT_TEST(test_gc_push_errors);
    CPPUNIT_TEST(test_gc_concurrent_push);
    CPPUNIT_TEST_SUITE_END();

    void test_item_destroy(){
        CPPUNIT_ASSERT_THROW(wasp::cgc::ConcurrentGCItem::destroy(NULL), wasp::cgc::NullPointerException);
    }

    void test_item(){
        wasp::cgc::ConcurrentGCItem* item_ptr = new wasp::cgc::ConcurrentGCItem(
            wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );
        CPPUNIT_ASSERT_THROW(new wasp::cgc::ConcurrentGCItem(NULL), wasp::cgc::NullPointerException);
        item_ptr->destroyable();
        wasp::cgc::ConcurrentGCItem::heap_destroy_fn(item_ptr);
    }

    void test_gc(){
        wasp::cgc::ConcurrentGarbageCollector* cgc = new wasp::cgc::ConcurrentGarbageCollector();
        CPPUNIT_ASSERT(cgc->items() == 0);
        delete cgc;
    }

    void test_gc_push_errors(){
        wasp::cgc::ConcurrentGCItem* item_ptr = new wasp::cgc::ConcurrentGCItem(
            wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );
        wasp::cgc::ConcurrentGarbageCollector* cgc = new wasp::cgc::ConcurrentGarbageCollector();

        CPPUNIT_ASSERT_THROW(cgc->push(NULL), wasp::cgc::NullPointerException);

        item_ptr->destroyable();
        CPPUNIT_ASSERT_THROW(cgc->push(item_ptr), wasp::cgc::InvalidItemState);
    }

    void test_gc_concurrent_push(){
        wasp::cgc::ConcurrentGarbageCollector* gc = new wasp::cgc::ConcurrentGarbageCollector();

        this->start_threads(
            "push_threads",
            1024,
            [gc](){
                TestWaspCGCTest::push_threaded_fn(10240, gc);
            },
            true
        );

        this->resume_threads("push_threads");

        this->join_threads("push_threads");
        delete gc;
    }

    static void push_threaded_fn(
        const size_t gc_items_per_thread, wasp::cgc::ConcurrentGarbageCollector* gc
    ){
        wasp::cgc::ConcurrentGCItem* items[gc_items_per_thread];

        for (size_t i=0; i < gc_items_per_thread; i++){
            items[i] = new wasp::cgc::ConcurrentGCItem(
                wasp::cgc::ConcurrentGCItem::heap_destroy_fn
            );
            gc->push(items[i]);
        }

        for (size_t i=0; i < gc_items_per_thread; i++){
            items[i]->destroyable();
        }

        gc->collect();
    }
};

};  // namespace wasp::cgc_test_case

CPPUNIT_TEST_SUITE_REGISTRATION(wasp::cgc_test_case::TestWaspCGCTest);
