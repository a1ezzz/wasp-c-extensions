#!/bin/bash -x

set -eu

CPP_EXEC="${1:?}"
ROOT_DIR="$(dirname $0)/.."
ROOT_DIR="$(realpath $ROOT_DIR)"

TEST_FILES="$(dirname $0)/*_test.cpp"
CGC_CPP_FILES="${ROOT_DIR}/wasp_c_extensions/_cgc/cgc.cpp"
CMCQUEUE_CPP_FILES="${ROOT_DIR}/wasp_c_extensions/_cmcqueue/cmcqueue.cpp"

g++ -fprofile-abs-path --coverage -I"${ROOT_DIR}" -I"${ROOT_DIR}/wasp_c_extensions" \
    ${TEST_FILES} \
    ${CGC_CPP_FILES} \
    ${CMCQUEUE_CPP_FILES} \
    -lcppunit -o "${CPP_EXEC}"  # -lcppunit should be at the end because of some g++ issue on jenkins worker

${CPP_EXEC}
