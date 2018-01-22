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

#include "hclib-internal.h"

static char* _hclib_action_print_op(hclib_op op) {
    switch (op) {
    case INIT:
	return "INIT";
	break;
    case BEGIN_FINISH:
	return "BEGIN_FINISH";
	break;
    case END_FINISH:
	return "END_FINISH";
	break;	
    case BEGIN_TASK:
	return "BEGIN_TASK";
	break;
    case END_TASK:
	return "END_TASK";
	break;
    default:
	abort();
    }
}

void _hclib_action_print_one_action(hclib_action *action) {
    if (action->op == INIT) {
	fprintf(stdout, "{'task': %d,'type': 'finish', 'id': %d, 'time': %d, 'op': '%s', 'args': []}\n", 0, 0, 0, _hclib_action_print_op(action->op));
    } else {
	char buf[16];
	sprintf(buf, "%d", action->arg);
	fprintf(stdout, "{'task': %d, 'type': 'finish', 'id': %d, 'time': %d, 'op': '%s', 'args': [%s]}\n",
		action->current_task,
		action->id,
		action->time,
		_hclib_action_print_op(action->op),
		(action->arg == 0)? "" : buf
	    );
    }
}
