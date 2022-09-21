
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_cgc/cgc.hpp"
#include "wasp_c_extensions/_cgc/smart_ptr.hpp"

namespace wasp::cgc_test_case {
    const size_t gc_threads_num = 1024;
    const size_t gc_items_per_thread = 10240;
    wasp::cgc::ConcurrentGarbageCollector* global_gc = NULL;

    std::atomic<bool> start_event_flag(false);
    std::condition_variable start_event_cv;
    std::mutex start_event_mutex;

class TestWaspCGCTest:
    public CppUnit::TestFixture
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
        std::thread* threads[wasp::cgc_test_case::gc_threads_num];

        wasp::cgc_test_case::global_gc = new wasp::cgc::ConcurrentGarbageCollector();

        for (size_t i=0; i < wasp::cgc_test_case::gc_threads_num; i++){
            threads[i] = new std::thread(TestWaspCGCTest::push_threaded_fn);
        }

        wasp::cgc_test_case::start_event_flag.store(true, std::memory_order_seq_cst);

        {
            std::lock_guard<std::mutex> lock(wasp::cgc_test_case::start_event_mutex);
            wasp::cgc_test_case::start_event_cv.notify_all();
        }

        for (size_t i=0; i < wasp::cgc_test_case::gc_threads_num; i++){
            threads[i]->join();
            delete threads[i];
        }

        delete wasp::cgc_test_case::global_gc;
        wasp::cgc_test_case::global_gc = NULL;
    }

    static void push_threaded_fn(){
        wasp::cgc::ConcurrentGCItem* items[wasp::cgc_test_case::gc_items_per_thread];

        while (! wasp::cgc_test_case::start_event_flag.load(std::memory_order_seq_cst)){
            std::unique_lock<std::mutex> lock(wasp::cgc_test_case::start_event_mutex);
            wasp::cgc_test_case::start_event_cv.wait(lock);
        }

        for (size_t i=0; i < wasp::cgc_test_case::gc_items_per_thread; i++){
            items[i] = new wasp::cgc::ConcurrentGCItem(
                wasp::cgc::ConcurrentGCItem::heap_destroy_fn
            );
            wasp::cgc_test_case::global_gc->push(items[i]);
        }

        for (size_t i=0; i < wasp::cgc_test_case::gc_items_per_thread; i++){
            items[i]->destroyable();
        }

        wasp::cgc_test_case::global_gc->collect();
    }
};

};  // namespace wasp::cgc_test_case

CPPUNIT_TEST_SUITE_REGISTRATION(wasp::cgc_test_case::TestWaspCGCTest);
