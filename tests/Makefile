
root_dir :=       ..
wasp_cpp_dir :=   ${root_dir}/wasp_c_extensions
wasp_cpp_files := ${wasp_cpp_dir}/_cgc/cgc.cpp ${wasp_cpp_dir}/_cgc/smart_ptr.cpp \
	${wasp_cpp_dir}/_cgc/smart_list.cpp ${wasp_cpp_dir}/_cmcqueue/cmcqueue.cpp \
	${wasp_cpp_dir}/_ev_loop/ev_loop.cpp ${wasp_cpp_dir}/_threads/event.cpp
wasp_hpp_files := $(wasp_cpp_files:.cpp=.hpp)

test_dir :=          ${root_dir}/tests
test_exec_runner :=  ${test_dir}/cpp-tests-exec
test_hpp_fixtures := ${test_dir}/tests_fixtures.hpp
test_cpp_files :=    $(wildcard ${test_dir}/*.cpp)
test_obj_files :=    $(patsubst %.cpp, %.o, $(test_cpp_files))

CC =             g++
COVERAGE_FLAGS = -fprofile-filter-files=.*\.cpp --coverage -fprofile-update=atomic
CFLAGS =         ${COVERAGE_FLAGS} ${GDB_FLAGS} -I$(root_dir) -I$(wasp_cpp_dir)
LDFLAGS =        -lcppunit -lpthread

abs_path_test_exec_runner := $(realpath $(test_exec_runner))

%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

$(test_exec_runner): $(wasp_cpp_files) $(test_obj_files) $(test_hpp_fixtures) \
$(wasp_hpp_files)
	$(CC) $(CFLAGS) $(LDFLAGS) $(wasp_cpp_files) $(test_obj_files) -o $(test_exec_runner)

all: $(test_exec_runner)

test:
	cd $(root_dir); $(abs_path_test_exec_runner)

# clean:
