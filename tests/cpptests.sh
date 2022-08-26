#!/bin/bash -x

set -eu

CPP_EXEC="${1:?}"
ROOT_DIR="$(dirname $0)/.."

TEST_FILES="${ROOT_DIR}/tests/*_test.cpp"
CGC_CPP_FILES="${ROOT_DIR}/wasp_c_extensions/_cgc/cgc.cpp"
CMCQUEUE_CPP_FILES="${ROOT_DIR}//wasp_c_extensions/_cmcqueue/cmcqueue.cpp"

g++ -I"${ROOT_DIR}" -I"${ROOT_DIR}/wasp_c_extensions" -lcppunit -o "${CPP_EXEC}" \
    ${TEST_FILES} \
    ${CGC_CPP_FILES} \
    ${CMCQUEUE_CPP_FILES}

${CPP_EXEC}
