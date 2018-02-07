/*
 * Copyright 2017 Rice University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HCLIB_FINISH_H
#define HCLIB_FINISH_H

#include "hclib-promise.h"
#include "hclib-atomics.h"

typedef struct finish_t {
    struct finish_t* parent;
    _Atomic int counter;
#if HCLIB_LITECTX_STRATEGY
    hclib_future_t ** finish_deps;
#endif /* HCLIB_LITECTX_STRATEGY */
#ifdef HCLIB_GENERATE_TRACE
    int id;
    hclib_task_t *task;
    _Atomic int timestamp;
#endif    
} finish_t;

#endif
