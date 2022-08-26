// wasp_c_extensions/_cgc/cgc.cpp
//
//Copyright (C) 2022 the wasp-c-extensions authors and contributors
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

#include "cgc.hpp"

using namespace wasp::cgc;

ConcurrentGCItem::ConcurrentGCItem(void (*fn)(ConcurrentGCItem*)):
    destroy_fn(fn),
    gc_ready(false),
    next(NULL),
    prev(NULL)
{
}

ConcurrentGCItem::~ConcurrentGCItem()
{}

ConcurrentGarbageCollector::ConcurrentGarbageCollector():
    head(NULL)
{}

ConcurrentGarbageCollector::~ConcurrentGarbageCollector()
{
    this->clear();
}

void ConcurrentGarbageCollector::push(ConcurrentGCItem*){
    // TODO: implement
}

void ConcurrentGarbageCollector::collect(ConcurrentGCItem* item)
{
    // TODO: implement
}

void ConcurrentGarbageCollector::clear(){
    // TODO: implement
}

ConcurrentGCItem* ConcurrentGarbageCollector::pop()
{
    // TODO: implement
    return NULL;
}
