
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_pqueue/pqueue.hpp"


namespace wasp::pqueue_test::test_case {
    const size_t threads_num = 100;  // TODO: increase!
    const size_t items_per_thread = 100;
    const size_t test_runs = 10;
    wasp::cgc::ConcurrentGarbageCollector* global_gc = NULL;
    wasp::pqueue::PriorityQueue* global_queue = NULL;
    std::atomic<size_t> pull_counter(0);

    std::atomic<bool> start_event_flag(false);
    std::condition_variable start_event_cv;
    std::mutex start_event_mutex;
}

class TestWaspPQueueTest: // TODO: rename?!
    public CppUnit::TestFixture
{

    CPPUNIT_TEST_SUITE(TestWaspPQueueTest);
    CPPUNIT_TEST(test);
    CPPUNIT_TEST(test_queue_concurrent_push);
    CPPUNIT_TEST_SUITE_END();

    void test(){
        wasp::cgc::ConcurrentGarbageCollector* gc = new wasp::cgc::ConcurrentGarbageCollector();
        wasp::pqueue::PriorityQueue* queue = new wasp::pqueue::PriorityQueue(gc);

        // TODO: test a different order

        wasp::pqueue::item_priority default_value = -1;

        int foo1 = 1;
        int foo2 = 2;
        int foo3 = 3;

        queue->push(foo1, &foo1);
        queue->push(foo2, &foo2);
        queue->push(foo3, &foo3);

        CPPUNIT_ASSERT(queue->next(default_value) == 3);
        CPPUNIT_ASSERT(queue->pull() == &foo3);

        CPPUNIT_ASSERT(queue->next(default_value) == 2);
        CPPUNIT_ASSERT(queue->pull() == &foo2);

        CPPUNIT_ASSERT(queue->next(default_value) == 1);
        CPPUNIT_ASSERT(queue->pull() == &foo1);

        CPPUNIT_ASSERT(queue->next(default_value) == default_value);
        CPPUNIT_ASSERT(queue->pull() == NULL);

        delete queue;
        delete gc;
    }

    void test_queue_concurrent_push(){
        for (size_t i=0; i < wasp::pqueue_test::test_case::test_runs; i++){
            queue_concurrent_push();
        }
    }

    void queue_concurrent_push(){
        std::thread* threads[wasp::pqueue_test::test_case::threads_num];

        wasp::pqueue_test::test_case::global_gc = new wasp::cgc::ConcurrentGarbageCollector();
        wasp::pqueue_test::test_case::global_queue = new wasp::pqueue::PriorityQueue(
            wasp::pqueue_test::test_case::global_gc
        );

        wasp::pqueue_test::test_case::pull_counter.store(0, std::memory_order_seq_cst);

        for (size_t i=0; i < wasp::pqueue_test::test_case::threads_num; i++){
            threads[i] = new std::thread(TestWaspPQueueTest::push_threaded_fn);
        }

        wasp::pqueue_test::test_case::start_event_flag.store(true, std::memory_order_seq_cst);

        {
            std::lock_guard<std::mutex> lock(wasp::pqueue_test::test_case::start_event_mutex);
            wasp::pqueue_test::test_case::start_event_cv.notify_all();
        }

        for (size_t i=0; i < wasp::pqueue_test::test_case::threads_num; i++){
            threads[i]->join();
            delete threads[i];
        }

        CPPUNIT_ASSERT(
            wasp::pqueue_test::test_case::pull_counter.load(std::memory_order_seq_cst) ==
            (wasp::pqueue_test::test_case::threads_num * wasp::pqueue_test::test_case::items_per_thread)
        );

        delete wasp::pqueue_test::test_case::global_queue;
        delete wasp::pqueue_test::test_case::global_gc;
        wasp::pqueue_test::test_case::global_queue = NULL;
        wasp::pqueue_test::test_case::global_gc = NULL;
    }

    static void push_threaded_fn(){
        while (! wasp::pqueue_test::test_case::start_event_flag.load(std::memory_order_seq_cst)){
            std::unique_lock<std::mutex> lock(wasp::pqueue_test::test_case::start_event_mutex);
            wasp::pqueue_test::test_case::start_event_cv.wait(lock);
        }

        for (size_t i=0; i < wasp::pqueue_test::test_case::items_per_thread; i++){
            wasp::pqueue_test::test_case::global_queue->push(i, &i);  // no matter what pointer we store =)
        }

        for (size_t i=0; i < wasp::pqueue_test::test_case::items_per_thread; i++){
            if (wasp::pqueue_test::test_case::global_queue->pull()){
                wasp::pqueue_test::test_case::pull_counter.fetch_add(1, std::memory_order_seq_cst);
            }
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestWaspPQueueTest);
