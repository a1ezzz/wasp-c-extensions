// wasp_c_extensions/_common/common.h
//
//Copyright (C) 2018 the wasp-c-extensions authors and contributors
//<see AUTHORS file>
//
//This file is part of wasp-c-extensions.
//
//Wasp-c-extensions is free software: you can redistribute it and/or modify
//it under the terms of the GNU Lesser General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//Wasp-c-extensions is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public License
//along with wasp-c-extensions.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __WASP_C_EXTENSIONS__COMMON_COMMON_H__
#define __WASP_C_EXTENSIONS__COMMON_COMMON_H__

#define __STR_FN__(M) #M
#define __STR_FN_CALL__(M) __STR_FN__(M)
#define __STR_PACKAGE_NAME__ __STR_FN_CALL__(__PACKAGE_NAME__)

#ifdef __WASP_DEBUG__
#define __WASP_DEBUG_PRINTF__(msg,...) printf("At file: "__FILE__", at line: %i: "msg"\n", __LINE__, ##__VA_ARGS__)
#else
#define __WASP_DEBUG_PRINTF__(msg,...)
#endif // __WASP_DEBUG__

#define __WASP_DEBUG_FN_CALL__ __WASP_DEBUG_PRINTF__("Function call: %s", __PRETTY_FUNCTION__)

#define __WASP_BEGIN_ALLOW_THREADS__ \
	__WASP_DEBUG_PRINTF__("Threads are going to be concurrent"); \
	Py_BEGIN_ALLOW_THREADS \
	__WASP_DEBUG_PRINTF__("Concurrent threads are allowed from now");

#define __WASP_END_ALLOW_THREADS__ \
	__WASP_DEBUG_PRINTF__("Concurrent threads are going to be disabled"); \
	Py_END_ALLOW_THREADS \
	__WASP_DEBUG_PRINTF__("Concurrent threads are disabled from now");

#define __PYINIT_MODULE_NAME_GEN_FN__(M) PyInit_ ## M
#define __PYINIT_MODULE_NAME_GEN_FN_CALL__(M) __PYINIT_MODULE_NAME_GEN_FN__(M)

#define __STR_THREADS_MODULE_NAME__ __STR_FN_CALL__(__THREADS_MODULE_NAME__)
#define __PYINIT_THREADS_MAIN_FN__ __PYINIT_MODULE_NAME_GEN_FN_CALL__(__THREADS_MODULE_NAME__)

#define __STR_ATOMIC_COUNTER_NAME__ __STR_FN_CALL__(__ATOMIC_COUNTER_NAME__)
#define __STR_PTHREAD_EVENT_NAME__ __STR_FN_CALL__(__PTHREAD_EVENT_NAME__)

#define __STR_QUEUE_MODULE_NAME__ __STR_FN_CALL__(__QUEUE_MODULE_NAME__)
#define __PYINIT_QUEUE_MAIN_FN__ __PYINIT_MODULE_NAME_GEN_FN_CALL__(__QUEUE_MODULE_NAME__)

#define __STR_MCQUEUE_NAME__ __STR_FN_CALL__(__MCQUEUE_NAME__)
#define __STR_MCQUEUE_SUBSCRIBER_NAME__ __STR_FN_CALL__(__MCQUEUE_SUBSCRIBER_NAME__)

#endif // __WASP_C_EXTENSIONS__COMMON_COMMON_H__
