
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_cgc/cgc.hpp"
#include "wasp_c_extensions/_cgc/smart_ptr.hpp"

#include "tests/tests_fixtures.hpp"

using namespace std::chrono_literals;


namespace wasp::smart_ptr_test_case {

class TestSmartDualPointer:
    public wasp::tests_fixtures::ThreadsRunner
{
    CPPUNIT_TEST_SUITE(TestSmartDualPointer);
    CPPUNIT_TEST(test_create_destroy);
    CPPUNIT_TEST(test_create_setup_destroy);
    CPPUNIT_TEST(test_multiple_setup_seq);
    CPPUNIT_TEST(test_multiple_setup_all);
    CPPUNIT_TEST_SUITE_END();

    static const size_t items_count = 100;
    static const size_t acquire_threads_count = 50;
    wasp::cgc::ConcurrentGarbageCollector* gc;
    wasp::cgc::ConcurrentGCItem* items[items_count];
    wasp::cgc::CGCSmartPointer<wasp::cgc::ConcurrentGCItem>* smart_ptrs[items_count];

    public:

    void test_create_destroy(){
        wasp::cgc::SmartDualPointer<wasp::cgc::ConcurrentGCItem> *dp =
            new wasp::cgc::SmartDualPointer<wasp::cgc::ConcurrentGCItem>();
        delete dp;
    }

    void test_create_setup_destroy(){
        wasp::cgc::SmartDualPointer<wasp::cgc::ConcurrentGCItem> *dp =
            new wasp::cgc::SmartDualPointer<wasp::cgc::ConcurrentGCItem>();

        CPPUNIT_ASSERT(dp->setup_next(this->smart_ptrs[0]));

        delete dp;
    }

    void test_multiple_setup_seq(){
        wasp::cgc::CGCSmartPointer<wasp::cgc::ConcurrentGCItem>* acquire_result = NULL;
        wasp::cgc::SmartDualPointer<wasp::cgc::ConcurrentGCItem> *dp =
            new wasp::cgc::SmartDualPointer<wasp::cgc::ConcurrentGCItem>();

        dp->setup_next(this->smart_ptrs[0]);

        for (size_t i = 1; i < this->items_count; i++){
            acquire_result = dp->acquire_next();
            CPPUNIT_ASSERT(acquire_result == this->smart_ptrs[i - 1]);
            CPPUNIT_ASSERT(dp->setup_next(this->smart_ptrs[i]));
            acquire_result->release();
        }

        acquire_result = dp->acquire_next();
        CPPUNIT_ASSERT(acquire_result == this->smart_ptrs[this->items_count - 1]);
        acquire_result->release();

        delete dp;
    }

    void test_multiple_setup_all(){
        wasp::cgc::CGCSmartPointer<wasp::cgc::ConcurrentGCItem>* acquire_result = NULL;
        wasp::cgc::SmartDualPointer<wasp::cgc::ConcurrentGCItem> *dp =
            new wasp::cgc::SmartDualPointer<wasp::cgc::ConcurrentGCItem>();

        for (size_t i = 0; i < this->items_count; i++){
            CPPUNIT_ASSERT(dp->setup_next(this->smart_ptrs[i]));
        }

        acquire_result = dp->acquire_next();
        CPPUNIT_ASSERT(acquire_result == this->smart_ptrs[this->items_count - 1]);
        acquire_result->release();

        delete dp;
    }

    public:
        void setUp(){
            this->gc = new wasp::cgc::ConcurrentGarbageCollector();

            for (size_t i = 0; i < this->items_count; i++){
                this->items[i] = new wasp::cgc::ConcurrentGCItem(wasp::cgc::ConcurrentGCItem::heap_destroy_fn);
                this->smart_ptrs[i] = new wasp::cgc::CGCSmartPointer<wasp::cgc::ConcurrentGCItem>(
                    &wasp::cgc::ConcurrentGCItem::heap_destroy_fn
                );

                assert(this->smart_ptrs[i]->init(this->items[i]));

                this->gc->push(this->smart_ptrs[i]);
                this->gc->push(this->items[i]);
            }
        };

        void tearDown(){
            for (size_t i = 0; i < this->items_count; i++){
                this->smart_ptrs[i]->destroyable();
                this->smart_ptrs[i]->release();
                this->items[i] = NULL;
                this->smart_ptrs[i] = NULL;
            }

            this->gc->collect();
            delete this->gc;
            this->gc = NULL;
        };
};

};  // namespace wasp::smart_ptr_test_case

CPPUNIT_TEST_SUITE_REGISTRATION(wasp::smart_ptr_test_case::TestSmartDualPointer);
