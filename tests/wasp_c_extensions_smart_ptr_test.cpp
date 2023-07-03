
#include <thread>

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_cgc/cgc.hpp"
#include "wasp_c_extensions/_cgc/smart_ptr.hpp"

#include "tests/tests_fixtures.hpp"

using namespace std::chrono_literals;

namespace wasp::smart_ptr_test_case {

class TestSmartPointer:
    public wasp::tests_fixtures::ThreadsRunner
{
    CPPUNIT_TEST_SUITE(TestSmartPointer);
    CPPUNIT_TEST(test_plain);
    CPPUNIT_TEST_PARAMETERIZED(test_simple_concurrency, wasp::tests_fixtures::sequence_generator<50>());
    CPPUNIT_TEST_SUITE_END();

    void test_plain(){
        wasp::cgc::ConcurrentGCItem* item_ptr = new wasp::cgc::ConcurrentGCItem(
            wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );

        wasp::cgc::SmartPointer<wasp::cgc::ConcurrentGCItem>* smart_ptr =
            new wasp::cgc::SmartPointer<wasp::cgc::ConcurrentGCItem>();

        CPPUNIT_ASSERT(smart_ptr->acquire() == NULL);  // it is not initialized at the moment

        CPPUNIT_ASSERT(smart_ptr->init(item_ptr));
        CPPUNIT_ASSERT(smart_ptr->init(item_ptr) == false);  // unable to initialize twice
        smart_ptr->release();

        delete smart_ptr;
        delete item_ptr;
    }

    void test_simple_concurrency(size_t){
        wasp::cgc::ConcurrentGCItem* item_ptr = new wasp::cgc::ConcurrentGCItem(
            wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );

        wasp::cgc::SmartPointer<wasp::cgc::ConcurrentGCItem>* smart_ptr =
            new wasp::cgc::SmartPointer<wasp::cgc::ConcurrentGCItem>();
        CPPUNIT_ASSERT(smart_ptr->init(item_ptr));

        this->start_threads(
            "acquire_threads",
            1000,
            [smart_ptr](){TestSmartPointer::acquire_release_thread_fn(smart_ptr, 10);}
        );

        std::this_thread::sleep_for(200ms);
        smart_ptr->release();

        this->join_threads("acquire_threads");
        delete smart_ptr;
        delete item_ptr;
    }

    static void acquire_release_thread_fn(
        wasp::cgc::SmartPointer<wasp::cgc::ConcurrentGCItem>* smart_ptr,
        size_t acquires_count
    ){

        if (smart_ptr->acquire()){

            for (size_t i=1; i < acquires_count; i++){
                  CPPUNIT_ASSERT(smart_ptr->acquire() != NULL);
            }

            for (size_t i=0; i < acquires_count; i++){
                smart_ptr->release();  // one extra release for a acquire at the beginning
            }
        }
    }
};

class TestCGCSmartPointer:
    public wasp::tests_fixtures::ThreadsRunner
{
    CPPUNIT_TEST_SUITE(TestCGCSmartPointer);
    CPPUNIT_TEST_PARAMETERIZED(test_concurrency, wasp::tests_fixtures::sequence_generator<50>());
    CPPUNIT_TEST_SUITE_END();

    void test_concurrency(size_t){
        wasp::cgc::ConcurrentGarbageCollector* gc = new wasp::cgc::ConcurrentGarbageCollector();

        wasp::cgc::ConcurrentGCItem* item_ptr = new wasp::cgc::ConcurrentGCItem(
            wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );

        wasp::cgc::CGCSmartPointer<wasp::cgc::ConcurrentGCItem>* gc_smart_ptr =
            new wasp::cgc::CGCSmartPointer<wasp::cgc::ConcurrentGCItem>(
                &wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );
        CPPUNIT_ASSERT(gc_smart_ptr->init(item_ptr));

        gc->push(item_ptr);
        gc->push(gc_smart_ptr);

        this->start_threads(
            "acquire_threads",
            1000,
            [gc_smart_ptr, gc](){
                TestCGCSmartPointer::acquire_release_thread_fn(gc_smart_ptr, 10);
                gc->collect();
            }
        );

        std::this_thread::sleep_for(200ms);
        gc_smart_ptr->release();

        this->join_threads("acquire_threads");
        delete gc;
    }

    static void acquire_release_thread_fn(
        wasp::cgc::CGCSmartPointer<wasp::cgc::ConcurrentGCItem>* gc_smart_ptr,
        size_t acquires_count
    ){
        if (gc_smart_ptr->acquire()){

            for (size_t i=1; i < acquires_count; i++){
                 CPPUNIT_ASSERT(gc_smart_ptr->acquire() != NULL);
            }

            for (size_t i=0; i < acquires_count; i++){
                gc_smart_ptr->release();  // one extra release for a acquire at the beginning
            }
        }
    }
};

};  // namespace wasp::smart_ptr_test_case

CPPUNIT_TEST_SUITE_REGISTRATION(wasp::smart_ptr_test_case::TestSmartPointer);

CPPUNIT_TEST_SUITE_REGISTRATION(wasp::smart_ptr_test_case::TestCGCSmartPointer);
