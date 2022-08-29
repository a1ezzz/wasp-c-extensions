
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_cgc/cgc.hpp"

namespace wasp::test_case {
    const size_t threads_num = 1024;
    const size_t items_per_thread = 10240;
    wasp::cgc::ConcurrentGarbageCollector* global_gc = NULL;

    std::atomic<bool> start_event_flag(false);
    std::condition_variable start_event_cv;
    std::mutex start_event_mutex;
}

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
    CPPUNIT_TEST(test_gc);
    CPPUNIT_TEST(test_gc_push_errors);
    CPPUNIT_TEST(test_gc_concurrent_push);
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

    void test_gc(){
        wasp::cgc::ConcurrentGarbageCollector* cgc = new wasp::cgc::ConcurrentGarbageCollector();
        CPPUNIT_ASSERT(cgc->items() == 0);
        delete cgc;
    }

    void test_gc_push_errors(){
        Item* item_ptr = Item::create();
        wasp::cgc::ConcurrentGarbageCollector* cgc = new wasp::cgc::ConcurrentGarbageCollector();

        CPPUNIT_ASSERT_THROW(cgc->push(NULL), wasp::cgc::NullPointerException);

        item_ptr->gc_ready.store(true, std::memory_order_seq_cst);  // without it the assertion fails
        CPPUNIT_ASSERT_THROW(cgc->push(item_ptr), wasp::cgc::InvalidItemState);
    }

    void test_gc_concurrent_push(){
        std::thread* threads[wasp::test_case::threads_num];

        wasp::test_case::global_gc = new wasp::cgc::ConcurrentGarbageCollector();

        for (size_t i=0; i < wasp::test_case::threads_num; i++){
            threads[i] = new std::thread(TestWaspCGCTest::push_threaded_fn);
        }

        wasp::test_case::start_event_flag.store(true, std::memory_order_seq_cst);

        {
            std::lock_guard<std::mutex> lock(wasp::test_case::start_event_mutex);
            wasp::test_case::start_event_cv.notify_all();
        }

        for (size_t i=0; i < wasp::test_case::threads_num; i++){
            threads[i]->join();
            delete threads[i];
        }

        delete wasp::test_case::global_gc;
        wasp::test_case::global_gc = NULL;
    }

    static void push_threaded_fn(){
        Item* items[wasp::test_case::items_per_thread];

        while (! wasp::test_case::start_event_flag.load(std::memory_order_seq_cst)){
            std::unique_lock<std::mutex> lock(wasp::test_case::start_event_mutex);
            wasp::test_case::start_event_cv.wait(lock);
        }

        for (size_t i=0; i < wasp::test_case::items_per_thread; i++){
            items[i] = Item::create();
            wasp::test_case::global_gc->push(items[i]);
        }

        for (size_t i=0; i < wasp::test_case::items_per_thread; i++){
            items[i]->gc_ready.store(true, std::memory_order_seq_cst);
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( TestWaspCGCTest);
