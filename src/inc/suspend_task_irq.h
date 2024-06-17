#ifndef __SUSPEND_TASK_H__
#define __SUSPEND_TASK_H__

#include <prem_task.h>

void suspend_task_irq(union memory_request_answer* memory_access, uint8_t from_irq);

#endif