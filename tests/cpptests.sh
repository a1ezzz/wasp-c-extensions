#!/bin/bash -x

set -eu

ROOT_DIR="$(dirname $0)/.."

CPP_EXEC="${ROOT_DIR}/${1:-cpp-tests-exec}"

TEST_FILES="$(dirname $0)/*_test.cpp"
CGC_CPP_FILES="${ROOT_DIR}/wasp_c_extensions/_cgc/cgc.cpp"
SMART_PTR_CPP_FILES="${ROOT_DIR}/wasp_c_extensions/_cgc/smart_ptr.cpp"
CMCQUEUE_CPP_FILES="${ROOT_DIR}/wasp_c_extensions/_cmcqueue/cmcqueue.cpp"
EVLOOP_CPP_FILES="${ROOT_DIR}/wasp_c_extensions/_ev_loop/ev_loop.cpp"
PQUEUE_CPP_FILES="${ROOT_DIR}/wasp_c_extensions/_pqueue/pqueue.cpp"
THREADS_CPP_FILES="${ROOT_DIR}/wasp_c_extensions/_threads/event.cpp"

COVERAGE_FLAGS="${COVERAGE_FLAGS:--fprofile-filter-files=.*\.cpp --coverage -fprofile-update=atomic}"
GDB_FLAGS="${GDB_FLAGS:- }"

g++ -Wall \
    ${COVERAGE_FLAGS} \
    ${GDB_FLAGS} \
    -I"${ROOT_DIR}" \
    -I"${ROOT_DIR}/wasp_c_extensions" \
    ${TEST_FILES} \
    ${CGC_CPP_FILES} \
    ${SMART_PTR_CPP_FILES} \
    ${CMCQUEUE_CPP_FILES} \
    ${EVLOOP_CPP_FILES} \
    ${PQUEUE_CPP_FILES} \
    ${THREADS_CPP_FILES} \
    -lcppunit -lpthread -o "${CPP_EXEC}"  # -lcppunit should be at the end because of some g++ issue on jenkins worker

${CPP_EXEC}
