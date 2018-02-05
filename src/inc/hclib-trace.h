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

#ifndef HCLIB_TRACE_H
#define HCLIB_TRACE_H

#include "hclib-atomics.h"

typedef enum _hclib_op {
    INIT = 0,
    BEGIN_FINISH = 1,
    END_FINISH = 2,
    BEGIN_TASK = 3,
    END_TASK= 4,
    END_INIT_TASK = 5
} hclib_op;

typedef struct _hclib_action {
    int current_task;
    hclib_op op;
    int id;
    int time;
    int arg;
} hclib_action;

static char* _hclib_action_print_op(hclib_op op);
void _hclib_action_print_one_action(hclib_action *action);
#endif
