// wasp_c_extensions/_threads/common.h
//
//Copyright (C) 2016 the wasp-c-extensions authors and contributors
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

#ifndef __WASP_C_EXTENSIONS__THREADS_COMMON_H__
#define __WASP_C_EXTENSIONS__THREADS_COMMON_H__

#define __STR_FN__(M) #M
#define __STR_FN_CALL__(M) __STR_FN__(M)
#define __STR_MODULE_NAME__ __STR_FN_CALL__(__MODULE_NAME__)
#define __STR_PACKAGE_NAME__ __STR_FN_CALL__(__PACKAGE_NAME__)
#define __STR_ATOMIC_COUNTER_NAME__ __STR_FN_CALL__(__ATOMIC_COUNTER_NAME__)

#ifdef __WASP_DEBUG__
#define __WASP_DEBUG_PRINTF__(msg) printf(msg"\n")
#else
#define __WASP_DEBUG_PRINTF__(msg)
#endif // __WASP_DEBUG__

#endif // __WASP_C_EXTENSIONS__THREADS_COMMON_H__
