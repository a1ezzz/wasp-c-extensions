
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
    
    controller.addListener(&result);
    controller.addListener(&progress);
    
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
    runner.run(controller);

    CppUnit::CompilerOutputter(&result, CppUnit::stdCOut()).write(); 
    
    return result.wasSuccessful() ? 0 : 1;
}
