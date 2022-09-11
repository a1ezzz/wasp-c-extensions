
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_cgc/cgc.hpp"

namespace wasp::cgc_test::test_case {
    const size_t gc_threads_num = 1024;
    const size_t gc_items_per_thread = 10240;
    wasp::cgc::ConcurrentGarbageCollector* global_gc = NULL;

    const size_t sp_simple_test_runs = 100;
    const size_t sp_simple_threads_num = 10240;
    const size_t sp_simple_acquire_per_thread = 3;

    const size_t sp_advanced_test_runs = 1000;
    const size_t sp_advanced_threads_num = 100;
    const size_t sp_advanced_acquire_per_thread = 3;

    wasp::cgc::SmartPointer* global_sp = NULL;

    std::atomic<bool> start_event_flag(false);
    std::condition_variable start_event_cv;
    std::mutex start_event_mutex;
}

class SampleCGCItem:
    public wasp::cgc::ConcurrentGCItem
{

    public:
        SampleCGCItem(void (*fn)(PointerDestructor*)):
            wasp::cgc::ConcurrentGCItem(fn)
        {}

        virtual ~SampleCGCItem(){};

        static SampleCGCItem* create(){
            return new SampleCGCItem(&SampleCGCItem::destroy);
        }

        static void destroy(wasp::cgc::PointerDestructor* ptr){
            delete ptr;
        }
};

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
        SampleCGCItem* item_ptr = SampleCGCItem::create();
        CPPUNIT_ASSERT_THROW(new SampleCGCItem(NULL), wasp::cgc::NullPointerException);
        item_ptr->destroyable();
        wasp::cgc::ConcurrentGCItem::destroy(item_ptr);
    }

    void test_gc(){
        wasp::cgc::ConcurrentGarbageCollector* cgc = new wasp::cgc::ConcurrentGarbageCollector();
        CPPUNIT_ASSERT(cgc->items() == 0);
        delete cgc;
    }

    void test_gc_push_errors(){
        SampleCGCItem* item_ptr = SampleCGCItem::create();
        wasp::cgc::ConcurrentGarbageCollector* cgc = new wasp::cgc::ConcurrentGarbageCollector();

        CPPUNIT_ASSERT_THROW(cgc->push(NULL), wasp::cgc::NullPointerException);

        item_ptr->destroyable();
        CPPUNIT_ASSERT_THROW(cgc->push(item_ptr), wasp::cgc::InvalidItemState);
    }

    void test_gc_concurrent_push(){
        std::thread* threads[wasp::cgc_test::test_case::gc_threads_num];

        wasp::cgc_test::test_case::global_gc = new wasp::cgc::ConcurrentGarbageCollector();

        for (size_t i=0; i < wasp::cgc_test::test_case::gc_threads_num; i++){
            threads[i] = new std::thread(TestWaspCGCTest::push_threaded_fn);
        }

        wasp::cgc_test::test_case::start_event_flag.store(true, std::memory_order_seq_cst);

        {
            std::lock_guard<std::mutex> lock(wasp::cgc_test::test_case::start_event_mutex);
            wasp::cgc_test::test_case::start_event_cv.notify_all();
        }

        for (size_t i=0; i < wasp::cgc_test::test_case::gc_threads_num; i++){
            threads[i]->join();
            delete threads[i];
        }

        delete wasp::cgc_test::test_case::global_gc;
        wasp::cgc_test::test_case::global_gc = NULL;
    }

    static void push_threaded_fn(){
        SampleCGCItem* items[wasp::cgc_test::test_case::gc_items_per_thread];

        while (! wasp::cgc_test::test_case::start_event_flag.load(std::memory_order_seq_cst)){
            std::unique_lock<std::mutex> lock(wasp::cgc_test::test_case::start_event_mutex);
            wasp::cgc_test::test_case::start_event_cv.wait(lock);
        }

        for (size_t i=0; i < wasp::cgc_test::test_case::gc_items_per_thread; i++){
            items[i] = SampleCGCItem::create();
            wasp::cgc_test::test_case::global_gc->push(items[i]);
        }

        for (size_t i=0; i < wasp::cgc_test::test_case::gc_items_per_thread; i++){
            items[i]->destroyable();
        }

        wasp::cgc_test::test_case::global_gc->collect();
    }
};

class TestSmartPointer:
    public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestSmartPointer);
    CPPUNIT_TEST(test_simple_concurrency);
    CPPUNIT_TEST(test_advanced_concurrency);
    CPPUNIT_TEST_SUITE_END();

    void test_simple_concurrency(){
        wasp::cgc_test::test_case::global_gc = new wasp::cgc::ConcurrentGarbageCollector();

        for (size_t i=0; i < wasp::cgc_test::test_case::sp_simple_test_runs; i++){
            this->threads_test(wasp::cgc_test::test_case::sp_simple_threads_num, TestSmartPointer::sp_simple_threaded_fn);
        }

        delete wasp::cgc_test::test_case::global_gc;
        wasp::cgc_test::test_case::global_gc = NULL;
    }

    void threads_test(const size_t threads_num, void (*f) ()){
        std::thread* threads[threads_num];
        SampleCGCItem* item_ptr = SampleCGCItem::create();
        wasp::cgc_test::test_case::global_sp = new wasp::cgc::SmartPointer(item_ptr);

        wasp::cgc_test::test_case::global_gc->push(item_ptr);

        for (size_t i=0; i < threads_num; i++){
            threads[i] = new std::thread(f);
        }

        wasp::cgc_test::test_case::global_sp->release();

        for (size_t i=0; i < threads_num; i++){
            threads[i]->join();
            delete threads[i];
        }

        delete wasp::cgc_test::test_case::global_sp;
        wasp::cgc_test::test_case::global_sp = NULL;
    }

    static void sp_simple_threaded_fn(){

        if (wasp::cgc_test::test_case::global_sp->acquire()){

            for (size_t i=1; i < wasp::cgc_test::test_case::sp_advanced_acquire_per_thread; i++){
                 CPPUNIT_ASSERT(wasp::cgc_test::test_case::global_sp->acquire() != NULL);
            }

            for (size_t i=0; i < wasp::cgc_test::test_case::sp_advanced_acquire_per_thread; i++){
                wasp::cgc_test::test_case::global_sp->release();
                wasp::cgc_test::test_case::global_gc->collect();
            }
        }
    }

    void test_advanced_concurrency(){
        wasp::cgc_test::test_case::global_gc = new wasp::cgc::ConcurrentGarbageCollector();

        for (size_t i=0; i < wasp::cgc_test::test_case::sp_advanced_test_runs; i++){
            this->threads_test(
                wasp::cgc_test::test_case::sp_advanced_threads_num, TestSmartPointer::sp_advanced_threaded_fn
            );
        }

        delete wasp::cgc_test::test_case::global_gc;
        wasp::cgc_test::test_case::global_gc = NULL;
    }

    static void sp_advanced_threaded_fn(){
        SampleCGCItem* item_ptr = SampleCGCItem::create();
        wasp::cgc_test::test_case::global_gc->push(item_ptr);

        if (wasp::cgc_test::test_case::global_sp->acquire()){

            for (size_t i=1; i < wasp::cgc_test::test_case::sp_advanced_acquire_per_thread; i++){
                CPPUNIT_ASSERT(wasp::cgc_test::test_case::global_sp->acquire() != NULL);
            }

            for (size_t i=0; i < wasp::cgc_test::test_case::sp_advanced_acquire_per_thread; i++){
                wasp::cgc_test::test_case::global_sp->release();
                wasp::cgc_test::test_case::global_gc->collect();
            }
        }

        while (! wasp::cgc_test::test_case::global_sp->replace(item_ptr)){};
        wasp::cgc_test::test_case::global_sp->release();
        wasp::cgc_test::test_case::global_gc->collect();

    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestWaspCGCTest);

CPPUNIT_TEST_SUITE_REGISTRATION(TestSmartPointer);
