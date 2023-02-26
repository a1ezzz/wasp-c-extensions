
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_cgc/smart_ptr.hpp"

#include "tests/tests_fixtures.hpp"


using namespace std::chrono_literals;


namespace wasp::smart_ptr_test_case {

class TestRenewableSmartLock:
    public wasp::tests_fixtures::ThreadsRunner
{
    CPPUNIT_TEST_SUITE(TestRenewableSmartLock);
    CPPUNIT_TEST(test_create_destroy);
    CPPUNIT_TEST(test_acquire_release);
    CPPUNIT_TEST_PARAMETERIZED(test_concurrency, wasp::tests_fixtures::sequence_generator<10>());
    CPPUNIT_TEST_SUITE_END();

    void test_create_destroy(){
        wasp::cgc::RenewableSmartLock* sl = new wasp::cgc::RenewableSmartLock();
        CPPUNIT_ASSERT(sl->release());
        delete sl;
    }

    void test_acquire_release(){
        wasp::cgc::RenewableSmartLock* sl = new wasp::cgc::RenewableSmartLock();

        for (size_t i = 0; i < 10; i++){
            CPPUNIT_ASSERT(sl->acquire());
            CPPUNIT_ASSERT(! sl->release());
        }

        CPPUNIT_ASSERT(sl->release());
        delete sl;
    }

    void test_concurrency(size_t){
        volatile bool acquire_is_on = true;
        wasp::cgc::RenewableSmartLock* sl = new wasp::cgc::RenewableSmartLock();

        this->start_threads(
            "acquire_threads",
            50,
            [&sl, &acquire_is_on](){TestRenewableSmartLock::acquire_thread_fn(sl, 10, &acquire_is_on);}
        );

        for (size_t i = 0; i < 10000; i++){
            sl->release();
            while (! sl->renew());
            std::this_thread::sleep_for(1ms);
        }

        acquire_is_on = false;
        this->join_threads("acquire_threads");

        CPPUNIT_ASSERT(sl->release());
        delete sl;
    }

    // TODO: add test where renew is called from concurrent threads

    public:
        static void acquire_thread_fn(
            wasp::cgc::RenewableSmartLock* sl, size_t sleep_ms, volatile bool* is_running
        ){
            do {
                if (sl->acquire()){
                    sl->release();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
            }
            while(*is_running);
        }
};

};  // namespace wasp::smart_ptr_test_case

CPPUNIT_TEST_SUITE_REGISTRATION(wasp::smart_ptr_test_case::TestRenewableSmartLock);
