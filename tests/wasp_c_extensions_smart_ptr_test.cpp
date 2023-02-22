
#include <thread>

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_cgc/cgc.hpp"
#include "wasp_c_extensions/_cgc/smart_ptr.hpp"

using namespace std::chrono_literals;

namespace wasp::smart_ptr_test_case {

    wasp::cgc::ConcurrentGarbageCollector* global_gc = NULL;

    const size_t sp_simple_test_runs = 50;
    const std::chrono::milliseconds sp_simple_sleep_time = std::chrono::duration_cast<std::chrono::milliseconds>(300ms);
    const size_t sp_simple_threads_num = 1000;
    const size_t sp_simple_acquire_per_thread = 3;

    const size_t sp_advanced_test_runs = 100;
    const std::chrono::milliseconds sp_advanced_sleep_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(300ms);
    const size_t sp_advanced_threads_num = 100;
    const size_t sp_advanced_acquire_per_thread = 3;

    wasp::cgc::SmartPointer<wasp::cgc::ConcurrentGCItem>* global_sp = NULL;

    const size_t gc_sp_advanced_test_runs = 50;
    const std::chrono::milliseconds gc_sp_sleep_time = std::chrono::duration_cast<std::chrono::milliseconds>(300ms);
    const size_t gc_sp_advanced_threads_num = 1000;
    const size_t gc_sp_advanced_acquire_per_thread = 3;

    wasp::cgc::CGCSmartPointer<wasp::cgc::ConcurrentGCItem>* global_gc_sp = NULL;

class TestSmartPointer:
    public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestSmartPointer);
    CPPUNIT_TEST(test_simple_concurrency);
    CPPUNIT_TEST_SUITE_END();

    void test_simple_concurrency(){
        wasp::smart_ptr_test_case::global_gc = new wasp::cgc::ConcurrentGarbageCollector();

        for (size_t i=0; i < wasp::smart_ptr_test_case::sp_simple_test_runs; i++){
            this->threads_test(
                wasp::smart_ptr_test_case::sp_simple_threads_num,
                TestSmartPointer::sp_simple_threaded_fn,
                wasp::smart_ptr_test_case::sp_simple_sleep_time
            );
        }

        delete wasp::smart_ptr_test_case::global_gc;
        wasp::smart_ptr_test_case::global_gc = NULL;
    }

    void threads_test(const size_t threads_num, void (*f) (), std::chrono::milliseconds sleep_time){
        std::thread* threads[threads_num];
        wasp::cgc::ConcurrentGCItem* item_ptr = new wasp::cgc::ConcurrentGCItem(
            wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );
        wasp::smart_ptr_test_case::global_sp = new wasp::cgc::SmartPointer<wasp::cgc::ConcurrentGCItem>();
        assert(wasp::smart_ptr_test_case::global_sp->init(item_ptr));

        wasp::smart_ptr_test_case::global_gc->push(item_ptr);

        for (size_t i=0; i < threads_num; i++){
            threads[i] = new std::thread(f);
        }

        std::this_thread::sleep_for(sleep_time);
        wasp::smart_ptr_test_case::global_sp->release();

        for (size_t i=0; i < threads_num; i++){
            threads[i]->join();
            delete threads[i];
        }

        delete wasp::smart_ptr_test_case::global_sp;
        wasp::smart_ptr_test_case::global_sp = NULL;
    }

    static void sp_simple_threaded_fn(){

        if (wasp::smart_ptr_test_case::global_sp->acquire()){

            for (size_t i=1; i < wasp::smart_ptr_test_case::sp_advanced_acquire_per_thread; i++){
                 CPPUNIT_ASSERT(wasp::smart_ptr_test_case::global_sp->acquire() != NULL);
            }

            for (size_t i=0; i < wasp::smart_ptr_test_case::sp_advanced_acquire_per_thread; i++){
                wasp::smart_ptr_test_case::global_sp->release();
                wasp::smart_ptr_test_case::global_gc->collect();
            }
        }
    }
};

class TestCGCSmartPointer:
    public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestCGCSmartPointer);
    CPPUNIT_TEST(test_concurrency);
    CPPUNIT_TEST_SUITE_END();

    void test_concurrency(){
        wasp::smart_ptr_test_case::global_gc = new wasp::cgc::ConcurrentGarbageCollector();

        for (size_t i=0; i < wasp::smart_ptr_test_case::gc_sp_advanced_test_runs; i++){
            this->threads_test();
        }

        delete wasp::smart_ptr_test_case::global_gc;
        wasp::smart_ptr_test_case::global_gc = NULL;
    }

    void threads_test(){
        std::thread* threads[wasp::smart_ptr_test_case::gc_sp_advanced_threads_num];
        wasp::cgc::ConcurrentGCItem* item_ptr = new wasp::cgc::ConcurrentGCItem(
            wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );
        wasp::smart_ptr_test_case::global_gc_sp = new wasp::cgc::CGCSmartPointer<wasp::cgc::ConcurrentGCItem>(
            &wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );
        assert(wasp::smart_ptr_test_case::global_gc_sp->init(item_ptr));

        wasp::smart_ptr_test_case::global_gc->push(item_ptr);
        wasp::smart_ptr_test_case::global_gc->push(wasp::smart_ptr_test_case::global_gc_sp);

        for (size_t i=0; i < wasp::smart_ptr_test_case::gc_sp_advanced_threads_num; i++){
            threads[i] = new std::thread(TestCGCSmartPointer::threaded_fn);
        }

        std::this_thread::sleep_for(wasp::smart_ptr_test_case::gc_sp_sleep_time);
        wasp::smart_ptr_test_case::global_gc_sp->destroyable();
        wasp::smart_ptr_test_case::global_gc_sp->release();

        for (size_t i=0; i < wasp::smart_ptr_test_case::gc_sp_advanced_threads_num; i++){
            threads[i]->join();
            delete threads[i];
        }

        wasp::smart_ptr_test_case::global_gc_sp = NULL;
    }

    static void threaded_fn(){
        if (wasp::smart_ptr_test_case::global_gc_sp->acquire()){

            for (size_t i=1; i < wasp::smart_ptr_test_case::gc_sp_advanced_acquire_per_thread; i++){
                 CPPUNIT_ASSERT(wasp::smart_ptr_test_case::global_gc_sp->acquire() != NULL);
            }

            for (size_t i=0; i < wasp::smart_ptr_test_case::gc_sp_advanced_acquire_per_thread; i++){
                wasp::smart_ptr_test_case::global_gc_sp->release();
                wasp::smart_ptr_test_case::global_gc->collect();
            }
        }
    }
};

};  // namespace wasp::smart_ptr_test_case

CPPUNIT_TEST_SUITE_REGISTRATION(wasp::smart_ptr_test_case::TestSmartPointer);

CPPUNIT_TEST_SUITE_REGISTRATION(wasp::smart_ptr_test_case::TestCGCSmartPointer);
