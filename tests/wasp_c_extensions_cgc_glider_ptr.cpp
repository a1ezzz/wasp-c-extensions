
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "wasp_c_extensions/_cgc/glider_ptr.hpp"

#include "tests/tests_fixtures.hpp"

namespace wasp::cgc_test_case {

class TestWaspGliderPtrNode:
    public wasp::tests_fixtures::GCThreadsRunnerFixture
{

    CPPUNIT_TEST_SUITE(TestWaspGliderPtrNode);
    CPPUNIT_TEST(test);
    CPPUNIT_TEST_SUITE_END();

    void test(){
        wasp::cgc::GliderPointerNode* glider_ptr_node = NULL;
        wasp::cgc::GliderPointer* glider_ptr = NULL;

        wasp::cgc::ConcurrentGCItem* item_ptr = new wasp::cgc::ConcurrentGCItem(
            wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );

        this->collector()->push(item_ptr);

        glider_ptr = new wasp::cgc::GliderPointer(this->collector(), item_ptr);

        glider_ptr_node = glider_ptr->head();
        CPPUNIT_ASSERT(glider_ptr_node);
        CPPUNIT_ASSERT(glider_ptr_node->acquire_ptr() == item_ptr);

        glider_ptr_node->release_ptr();
        glider_ptr_node->release_node();

        delete glider_ptr;
    };
};


class TestWaspGliderPtr:
    public wasp::tests_fixtures::GCThreadsRunnerFixture
{

    class IntHeapItem:
        public wasp::cgc::ConcurrentGCItem
    {
	public:

	    IntHeapItem(int v, wasp::cgc::ConcurrentGarbageCollector* gc):
		    wasp::cgc::ConcurrentGCItem(wasp::cgc::ConcurrentGCItem::heap_destroy_fn),
		    value(v)
	    { gc->push(this);};

	    int value;
    };

    CPPUNIT_TEST_SUITE(TestWaspGliderPtr);
    CPPUNIT_TEST(simple_test);
    CPPUNIT_TEST_PARAMETERIZED(advanced_test, {false, true});
    CPPUNIT_TEST(test_heap_value);
    CPPUNIT_TEST_PARAMETERIZED(test_simple_concurrency, wasp::tests_fixtures::sequence_generator<10>());
    CPPUNIT_TEST_SUITE_END();

    void simple_test(){
        wasp::cgc::GliderPointer* glider_ptr = NULL;
        wasp::cgc::ConcurrentGCItem* item_ptr = new wasp::cgc::ConcurrentGCItem(
            wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );

        this->collector()->push(item_ptr);

        glider_ptr = new wasp::cgc::GliderPointer(this->collector(), item_ptr);

        delete glider_ptr;
    };

    void advanced_test(bool r){
        wasp::cgc::GliderPointerNode* glider_ptr_node = NULL;
        wasp::cgc::GliderPointer* glider_ptr = NULL;
        wasp::cgc::ConcurrentGCItem* first_item_ptr = new wasp::cgc::ConcurrentGCItem(
            wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );
        wasp::cgc::ConcurrentGCItem* second_item_ptr = new wasp::cgc::ConcurrentGCItem(
            wasp::cgc::ConcurrentGCItem::heap_destroy_fn
        );

        this->collector()->push(first_item_ptr);
        this->collector()->push(second_item_ptr);

        glider_ptr = new wasp::cgc::GliderPointer(this->collector(), first_item_ptr);
        glider_ptr_node = glider_ptr->head();
        glider_ptr->replace_head(second_item_ptr, (r ? glider_ptr_node : NULL));
        glider_ptr_node->release_node();

        delete glider_ptr;
    };
    
    void test_heap_value(){
    	IntHeapItem* heap_value = new IntHeapItem(0, this->collector());
	    wasp::cgc::GliderPointer* glider_ptr = new wasp::cgc::GliderPointer(this->collector(), heap_value);
	    delete glider_ptr;
    };

    void test_simple_concurrency(size_t){
        const size_t threads_number = 10;
        const size_t iterations_per_thread = 50;

        wasp::cgc::GliderPointer* glider_ptr = new wasp::cgc::GliderPointer(
            this->collector(), new IntHeapItem(0, this->collector())
        );

        this->start_threads(
            "sorted_push_threads",
            threads_number,
            [threads_number, iterations_per_thread, this, glider_ptr](size_t i){
                wasp::cgc::GliderPointerNode* head_ptr = NULL;
                IntHeapItem *next_value = NULL, *head_int = NULL;
                int iter_value = 0;

                for (size_t j=0; j < iterations_per_thread; j++){
                    iter_value = (j * threads_number) + i + 1;

                    next_value = new IntHeapItem(iter_value, this->collector());

                    while (true){
                        head_ptr = glider_ptr->head();
                        head_int = dynamic_cast<IntHeapItem*>(head_ptr->acquire_ptr());

                        if (head_int->value == (iter_value - 1)){
                            CPPUNIT_ASSERT(glider_ptr->replace_head(next_value, head_ptr));
                            head_ptr->release_ptr();
                            head_ptr->release_node();
                            break;
                        }

                        head_ptr->release_ptr();
                        head_ptr->release_node();
                    }
                }
            },
            true
        );

        this->resume_threads("sorted_push_threads");
        this->join_threads("sorted_push_threads");

        delete glider_ptr;
    }
};

};  // namespace wasp::cgc_test_case

CPPUNIT_TEST_SUITE_REGISTRATION(wasp::cgc_test_case::TestWaspGliderPtrNode);

CPPUNIT_TEST_SUITE_REGISTRATION(wasp::cgc_test_case::TestWaspGliderPtr);
