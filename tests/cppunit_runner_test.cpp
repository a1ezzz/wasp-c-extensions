
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>

int main(int argc, char* argv[])
{
    CppUnit::TestResult controller;
    CppUnit::TestResultCollector result;
    CppUnit::BriefTestProgressListener progress;
    CppUnit::TestRunner runner;
    const char* test_name_c_ptr = std::getenv("WASP_CPP_TESTS");
    std::string test_name_str(test_name_c_ptr ? test_name_c_ptr : "");

    controller.addListener(&result);
    controller.addListener(&progress);
    
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
    runner.run(controller, test_name_str);

    CppUnit::CompilerOutputter(&result, CppUnit::stdCOut()).write(); 
    
    return result.wasSuccessful() ? 0 : 1;
}
